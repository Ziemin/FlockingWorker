import improbable.build._
import improbable.build.fabric._
import improbable.build.unity._
import improbable.build.util.Versions
import improbable.sdk.SdkInfo

object BuildConfiguration extends improbable.build.ImprobableBuild(
  projectName = "buildingdemo",
  organisation = "demoteam",
  version = Versions.fetchVersion("BuildingDemo"),
  isLibrary = false,
  buildSettings = Seq(FabricBuildSettings(), UnityPlayerProject()),
  dependencies = List(new UnitySimulationLibrary("improbable", "core-library", SdkInfo.version))
)
