package behaviours.controls

import demoteam.Player
import improbable.corelib.util.EntityOwnerDelegation.entityOwnerDelegation
import improbable.corelibrary.transforms.TransformState
import improbable.papi.entity.{Entity, EntityBehaviour}
import improbable.papi.world.World

class PlayerBehaviour(entity: Entity, world:World) extends EntityBehaviour {
  entity.delegateStateToOwner[Player]
  entity.delegateStateToOwner[TransformState]
}
