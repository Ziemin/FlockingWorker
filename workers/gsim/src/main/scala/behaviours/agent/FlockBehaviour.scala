package behaviours.agent

import demoteam.{Transform, FlockOutput}
import demoteam.buildingdemo.launcher.FlockingWorkerConstraint
import improbable.papi.entity.{EntityBehaviour, Entity}
import improbable.papi.world.World

class FlockBehaviour(entity:Entity, world:World) extends EntityBehaviour {
  override def onReady(): Unit = {
    entity.addEngineConstraint(FlockingWorkerConstraint)
    entity.delegateState[Transform](FlockingWorkerConstraint)
  }
}
