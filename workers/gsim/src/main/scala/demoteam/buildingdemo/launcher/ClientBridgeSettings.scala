package demoteam.buildingdemo.launcher

import improbable.fapi.bridge.{PerEntityOrderedStateUpdateQos, ConstantEngineLoadPolicy, BridgeSettings, BridgeSettingsResolver}
import improbable.fapi.network.RakNetLinkSettings
import improbable.unity.fabric.{AuthoritativeEntityOnly, VisualEngineConstraint}
import improbable.unity.fabric.bridge.ClientAssetContextDiscriminator
import improbable.unity.fabric.engine.EnginePlatform
import improbable.unity.fabric.engine.EnginePlatform._
import improbable.unity.fabric.satisfiers.{SatisfySingleConstraint, SatisfySpecificEngine, AggregateSatisfiers}

object ClientBridgeSettings extends BridgeSettingsResolver {

  private val CLIENT_ENGINE_BRIDGE_SETTINGS = BridgeSettings(
    ClientAssetContextDiscriminator(),
    RakNetLinkSettings(),
    EnginePlatform.UNITY_CLIENT_ENGINE,
    AggregateSatisfiers(
      SatisfySpecificEngine,
      SatisfySingleConstraint(VisualEngineConstraint)
    ),
    AuthoritativeEntityOnly(2),
    ConstantEngineLoadPolicy(0.5),
    PerEntityOrderedStateUpdateQos
  )

  private val ANDROID_CLIENT_ENGINE_BRIDGE_SETTINGS = CLIENT_ENGINE_BRIDGE_SETTINGS.copy(enginePlatform = UNITY_ANDROID_CLIENT_ENGINE)

  private val bridgeSettings = Map[String, BridgeSettings](
    UNITY_CLIENT_ENGINE -> CLIENT_ENGINE_BRIDGE_SETTINGS,
    UNITY_ANDROID_CLIENT_ENGINE -> ANDROID_CLIENT_ENGINE_BRIDGE_SETTINGS
  )

  override def engineTypeToBridgeSettings(engineType: String, metadata: String): Option[BridgeSettings] = {
    bridgeSettings.get(engineType)
  }
}