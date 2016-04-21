package behaviours.agent

import com.typesafe.scalalogging.Logger
import demoteam._
import improbable.corelib.math.Quaternion
import improbable.math.{Vector3f, Vector3d}
import improbable.papi.entity.{EntitySnapshot, Entity, EntityBehaviour}
import improbable.papi.world.World
import templates.BirdConstants
import BirdConstants._
import FlockGsimBehaviour._
import scala.concurrent.duration._

class FlockGsimBehaviour(entity: Entity, world: World, logger: Logger, state: TransformWriter, stats: FlockStatsInterface) extends EntityBehaviour {

  def sqr[T](v: T)(implicit num: Numeric[T]): T = {
    import num._
    v * v
  }

  def quatFromTo(unitFrom: Vector3d, unitTo: Vector3d): Quaternion = {

    val perp = unitFrom.cross(unitTo)
    if (perp.magnitudeSquared < ep) {
      Quaternion.identity
    }
    else {
      val unitPerp = perp.normalised
      val cosAng = unitFrom.dot(unitTo)
      val ang = math.acos(math.min(math.max(cosAng, -1.0), 1.0))
      Quaternion.fromAngleAxis(ang.toDegrees.toFloat, unitPerp)
    }
  }

  def quatAxisAngle(q: Quaternion): Option[(Vector3f, Float)] = {
    val ang = 2.0 * math.acos(q.w)
    val sinHalfAngle = math.sin(ang / 2)
    if (math.abs(sinHalfAngle) > ep) {
      val ax = Vector3d(q.x, q.y, q.z) / sinHalfAngle
      Some((ax.toVector3f, ang.toDegrees.toFloat))
    }
    else {
      None
    }
  }

  def approxEquals(v0: Double, v1: Double, ep: Double): Boolean = {
    math.abs(v0 - v1) < ep
  }

  def quatEquals(q0: Quaternion, q1: Quaternion, ep: Double): Boolean = {
    approxEquals(q0.x, q1.x, ep) &&
      approxEquals(q0.y, q1.y, ep) &&
      approxEquals(q0.z, q1.z, ep) &&
      approxEquals(q0.w, q1.w, ep)
  }

  def vec3dEquals(v0: Vector3d, v1: Vector3d, ep: Double): Boolean = {
    approxEquals(v0.x, v1.x, ep) &&
      approxEquals(v0.y, v1.y, ep) &&
      approxEquals(v0.z, v1.z, ep)
  }

  def shouldConsiderEntity(snap: EntitySnapshot) : Boolean = {

    val lineTo = snap.position - entity.position
    state.forward.dot(lineTo.toVector3f)>0.0f
  }

  override def onReady(): Unit = {

    val ln2 = math.log(2)
    val wFlock = entity.watch[Flock]

    val targetFPS = 8
    val targetFrameTime = 1.0f / targetFPS

    world.timing.every(targetFrameTime.seconds) {

      val time0 = System.nanoTime()

      val navigators = wFlock.searchRange.map {
        range =>
          world.entities.find(entity.position, range, Set(birdTag)).filter { snap =>
            snap.entityId != entity.entityId && shouldConsiderEntity(snap)
          }
      }.getOrElse(Nil)


      val closestNav = navigators.sortBy {
        nav =>
          (nav.position - entity.position).magnitudeSquared
      }

      val closestN = wFlock.numberToConsider.map(closestNav.take).getOrElse(Seq.empty)

      case class Accumulator(avePos: Vector3d = Vector3d.zero, aveHeading: Vector3f = Vector3f.zero, repulsionDelta: Vector3f = Vector3f.zero)

      val nClosest = closestN.size
      val oneOnN = if (nClosest > 0) {
        1.0f / nClosest
      } else {
        0.0f
      }

      val accumulator = closestN.foldLeft(Accumulator()) {
        (acc, neighbour) =>
          val lineAway = (entity.position - neighbour.position).toVector3f

          val repulsionDelta = acc.repulsionDelta + wFlock.repelSeparationForHalf.map {
            repelSeparationForHalf =>
              val sepK = ln2 / sqr(repelSeparationForHalf)
              val mag = math.exp(-sepK * lineAway.magnitudeSquared)
              if (lineAway.magnitudeSquared > ep) {
                lineAway.normalised
              } else {
                Vector3f.zero
              } * mag.toFloat
          }.getOrElse(Vector3f.zero)

          val avePos = acc.avePos + neighbour.position.toVector3d * oneOnN
          val aveHeading = acc.aveHeading + neighbour.get[Transform].get.velocity * oneOnN

          Accumulator(avePos, aveHeading, repulsionDelta)
      }

      val steeringVector0 = if (nClosest > 0) {
        val repelCoef = wFlock.repelCoefficient.getOrElse(0.0f)
        val followCoef = wFlock.followCoefficient.getOrElse(0.0f)
        val attractCoef = wFlock.attractCoefficient.getOrElse(0.0f)
        val vel = state.velocity

        (accumulator.avePos - entity.position.toVector3d).toVector3f * attractCoef +
          (accumulator.aveHeading - vel) * followCoef +
          accumulator.repulsionDelta * repelCoef
      }
      else {
        Vector3f.zero
      }

      val maxDist = 192.0
      val toOrigin = Vector3f.zero - state.position.toVector3d.toVector3f
      val distribution = math.pow(toOrigin.magnitudeSquared / (maxDist * maxDist), 16.0)
      val steeringVector1 = steeringVector0 + (if (toOrigin.magnitudeSquared > ep) {
        toOrigin.normalised * distribution.toFloat
      } else {
        Vector3f.zero
      })

      val minHeight = 10.0
      val maxHeight = 30.0
      val height = state.position.toVector3d.dot(Vector3d.unitY)
      val steeringVector2 = steeringVector1 - (if ((height < minHeight && steeringVector1.y < 0.0f) || (height > maxHeight && steeringVector1.y > 0.0f)) {
        steeringVector1 * Vector3f.unitY * 2
      }
      else {
        Vector3f.zero
      })

      val steeringVector = steeringVector2

      val newFwd = if (steeringVector.magnitudeSquared > ep) {
        val maxAngle = wFlock.maxTurnDegreesPerSecond.getOrElse(5.0f).toDouble.toRadians.toFloat
        val targetFacing = steeringVector.normalised
        val ey = state.forward.cross(targetFacing.toVector3d)
        if (ey.magnitudeSquared > ep) {
          val ez = state.forward
          val ex = ey.normalised.cross(ez.toVector3d).toVector3f
          val cosAng = targetFacing.dot(state.forward)
          val ang = math.acos(math.max(math.min(cosAng, 1.0), -1.0))
          val angLimited = math.min(ang, maxAngle)
          (ez * math.cos(angLimited).toFloat + ex * math.sin(angLimited).toFloat).normalised
        }
        else {
          state.forward
        }
      }
      else {
        state.forward
      }

      val newVel = newFwd * wFlock.speed.getOrElse(5.0f)
      val newPos = state.position + newVel * targetFrameTime

      state.update.position(newPos).forward(newFwd).velocity(newVel).finishAndSend()

      val time1 = System.nanoTime()
      val ns: Double = (time1 - time0) / 1e9

      stats.addStat(ns.toDouble)
    }
  }
}

object FlockGsimBehaviour {
  val ep = 0.001
}
