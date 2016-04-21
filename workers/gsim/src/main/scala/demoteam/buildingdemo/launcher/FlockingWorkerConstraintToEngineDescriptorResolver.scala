package demoteam.buildingdemo.launcher

import improbable.fapi.engine.{EngineDescriptor, ConstraintToEngineDescriptorResolver}
import improbable.papi.engine.EngineConstraint

class FlockingWorkerConstraintToEngineDescriptorResolver extends ConstraintToEngineDescriptorResolver {
  override def getEngineDescriptorForConstraint(engineConstraint: EngineConstraint): Option[EngineDescriptor] = {
    engineConstraint match {
      case FlockingWorkerConstraint => Some(FlockingWorkerDescriptor)
      case _ => None
    }
  }
}
object FlockingWorkerConstraintToEngineDescriptorResolver extends FlockingWorkerConstraintToEngineDescriptorResolver