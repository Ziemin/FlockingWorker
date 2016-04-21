package templates

import behaviours.agent.{FlockStatsBehaviour, FlockGsimBehaviour, FlockBehaviour}
import demoteam._
import improbable.corelib.natures.{BaseNature, NatureApplication, NatureDescription}
import improbable.math.{Vector3d, Vector3f, Coordinates}
import improbable.papi.entity.EntityPrefab
import improbable.papi.entity.behaviour.EntityBehaviourDescriptor

import BirdConstants._

object BirdNature extends NatureDescription {

  override def dependencies: Set[NatureDescription] = Set(BaseNature)

  override def activeBehaviours: Set[EntityBehaviourDescriptor] = Set(
    descriptorOf[FlockBehaviour]
    //descriptorOf[FlockGsimBehaviour],
    //descriptorOf[FlockStatsBehaviour]
  )

  def apply(position: Coordinates, speed: Float): NatureApplication = application(
    natures = Seq(BaseNature(EntityPrefab("Bird"), List(birdTag), isPhysical = false, isVisual = true)),
    states = Seq(
      Transform(position, Vector3f.unitZ, Vector3f.zero),
      Flock(  attractCoefficient = 5.0f,
              followCoefficient = 3.0f,
              repelCoefficient = 6.0f,
              repelSeparationForHalf = 3.0f,
              searchRange = 18.0f,
              numberToConsider = 7,
              speed = speed,
              drag = 0.25f,
              velocitySpring = 28.0f,
              verticalConfinementCoefficient = 12.0f,
              maxTurnDegreesPerSecond = 5.0f
          )
      ,FlockStats(Nil)
    )
  )
}

object BirdConstants {
  val birdTag = "Bird"
}
