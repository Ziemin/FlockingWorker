package improbable.apps

import com.typesafe.scalalogging.Logger
import improbable.papi.world.AppWorld
import improbable.papi.worldapp.{WorldAppLifecycle, WorldApp}

class SetupWorld(val world: AppWorld, val logger: Logger, val lifecycle: WorldAppLifecycle) extends WorldApp {
//  var numBase = 12
//  var numHeight = 6
//  var helpr = new CylinderBuildHelper(1.0f, numBase, 1.05f, 1.05f)
//  (0 until numBase * numHeight).foreach {
//    iblock =>
//      world.entities.spawnEntity(BlockNature(helpr.getPositionForBlockAtIndex(iblock).toCoordinates, helpr.getRotationForBlockAtIndex(iblock)))
//  }

//  world.entities.spawnEntity(BuilderNature(Coordinates.zero, Vector3d.zero))
}
