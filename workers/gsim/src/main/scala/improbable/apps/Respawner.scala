package improbable.apps

import improbable.logging.DistributedArchLogging
import improbable.math.{Vector3d, Coordinates}
import improbable.papi.world.AppWorld
import improbable.papi.world.messaging.CustomMsg
import improbable.papi.worldapp.WorldAppDescriptor
import templates._

import scala.concurrent.duration._

import Respawner._

case class SpawnStuff(origin:Coordinates) extends CustomMsg

class Respawner(val world: AppWorld) extends DistributedArchLogging {
  import Parameters._

  world.messaging.onReceive{
    case SpawnStuff(origin) =>
      spawnBirds(origin.toVector3d + Vector3d.unitY*25.0)
  }

  def restart(): Unit = {
    logger.info("Restarting simulation")

    destroyEverything()
    world.timing.after(2.seconds) {
      destroyEverything()
    }
    world.timing.after(3.seconds) {
      world.messaging.sendToApp(WorldAppDescriptor.forClass[SimulationReloader].name, SpawnStuff(Coordinates.zero))
    }

  }

  def destroyEverything(): Unit = {
  }

  def spawnBirds(origin: Vector3d) : Unit = {

    val sizeX = bird_cage_size.get().toFloat
    val sizeZ = sizeX
    val nx = num_bird_cells.get()
    val nz = nx
    val posns = (0 until nx*nz).map {
      i=>
        val ix = i%nx
        val iz = i/nx

        origin + Vector3d.unitX*(ix-(nx-1)/2.0)*sizeX/(nx-1) + Vector3d.unitZ*(iz-(nz-1)/2.0)*sizeZ/(nz-1)
    }

    posns.foreach {
      modOrigin =>
        val nbirds = number_of_birds.get()
        if (nbirds > 0) {
          val delay = cube_impulse_interval_millis.get() / number_of_birds.get.toDouble
          (0 until nbirds).foreach {
            i =>
              Thread.sleep(delay.floor.toLong, ((delay - delay.floor) * 1000).toInt)
              val spawnPos = modOrigin.toCoordinates +
                randomPosWithinRadius(bird_spawn_radius.get().toDouble, 0.0f).toVector3d +
                Vector3d.unitY * ((i % 16) / 16.0) * 5

              val interp = Math.random().toFloat
              val speedMin = 0.9f
              val speedMax = 1.1f
              val randFactor = speedMax*interp + speedMin*(1-interp)
              world.entities.spawnEntity(BirdNature(spawnPos, randFactor*5.0f))
          }
        }
    }
  }
}

object Respawner {
  def randomPosWithinRadius(rMax:Double, rMin:Double): Coordinates = {
    val angle = Math.random()*2*Math.PI
    val radius = rMin + (math.max(rMax, rMin)-rMin)*Math.sqrt(Math.random())
    Coordinates(math.sin(angle)*radius,0,math.cos(angle)*radius)
  }
}