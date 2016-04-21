package demoteam.buildingdemo.launcher

import improbable.apps.{BuildingDemoWorldAppList}
import improbable.dapi.{Launcher, LaunchConfig}
import improbable.fapi.bridge._
import improbable.fapi.engine.CompositeEngineDescriptorResolver
import improbable.papi.worldapp.WorldApp
import improbable.unity.fabric.bridge.{UnityClientBridgeSettings, UnityFSimBridgeSettings}
import improbable.unity.fabric.engine.{DownloadableUnityConstraintToEngineDescriptorResolver}

/**
 * These are the engine startup configs.
 *
 * ManualEngineStartup will not start an engines when you start the game.
 * AutomaticEngineStartup will automatically spool up engines as you need them.
 */
object SimulationLaunchWithManualEngineStartupConfig extends SimulationLaunchConfigWithApps(dynamicallySpoolUpEngines = false)

object SimulationLaunchWithAutomaticEngineStartupConfig extends SimulationLaunchConfigWithApps(dynamicallySpoolUpEngines = true)

/**
 * Use this class to specify the list of apps you want to run when the game starts.
 */
class SimulationLaunchConfigWithApps(dynamicallySpoolUpEngines: Boolean) extends SimulationLaunchConfig(BuildingDemoWorldAppList.apps, dynamicallySpoolUpEngines)

class SimulationLaunchConfig(appsToStart: Seq[Class[_ <: WorldApp]],
                             dynamicallySpoolUpEngines: Boolean) extends LaunchConfig(
  appsToStart,
  dynamicallySpoolUpEngines,
  DefaultBridgeSettingsResolver,
  BuildingDemoConstraintResolver)

object DefaultBridgeSettingsResolver extends CompositeBridgeSettingsResolver(
  ClientBridgeSettings,
  UnityFSimBridgeSettings,
  FlockingWorkerBridgeSettings
)

object DefaultConstraintEngineDescriptorResolver extends CompositeEngineDescriptorResolver(
  DownloadableUnityConstraintToEngineDescriptorResolver
)
object BuildingDemoConstraintResolver extends CompositeEngineDescriptorResolver(
  DownloadableUnityConstraintToEngineDescriptorResolver,
  FlockingWorkerConstraintToEngineDescriptorResolver
)

object SimulationLauncherWithManualEngines extends SimulationLauncher(SimulationLaunchWithManualEngineStartupConfig)

object SimulationLauncherWithAutomaticEngines extends SimulationLauncher(SimulationLaunchWithAutomaticEngineStartupConfig)

class SimulationLauncher(launchConfig: LaunchConfig) extends App {
  val options = Seq(
    "--entity_activator=improbable.corelib.entity.CoreLibraryEntityActivator",
    "--resource_based_config_name=one-gsim-one-jvm"
    ,"--engine_automatic_scaling_enabled=false"
    ,"--game_chunk_size=16"
//    ,"--engine_range=1000"
//    ,"--spatial_index_grid_size=5"
//    ,"--snapshot_write_period_seconds=0"
//    ,"--use_immutable_entity_search_space=true"
  )
  Launcher.startGame(launchConfig, options: _*)
}
