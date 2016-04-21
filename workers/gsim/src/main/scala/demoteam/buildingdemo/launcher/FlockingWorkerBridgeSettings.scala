package demoteam.buildingdemo.launcher

import improbable.fapi.bridge._
import improbable.fapi.network.MultiplexTcpLinkSettings
import improbable.serialization.KryoSerializable
import improbable.unity.fabric.satisfiers.SatisfySingleConstraint

object FlockingWorkerBridgeSettings extends BridgeSettingsResolver {

  val WORKER_TYPE = "FlockingWorker"

  object ContextDescriminator extends AssetContextDiscriminator {
    override def assetContextForEntity(entity: EngineEntity): String = {
      ""
    }
  }

  object RadialInterestPolicy extends EntityInterestPolicy with KryoSerializable {
    val Radius = 1

    override def interestTypeFor(entity: EngineEntity): Option[InterestType] = {
      Some(RadialInterest(Radius))
    }
  }

  private val FLOCKING_ENGINE_BRIDGE_SETTINGS = BridgeSettings(
    ContextDescriminator,
    MultiplexTcpLinkSettings(4),
    WORKER_TYPE,
    SatisfySingleConstraint(FlockingWorkerConstraint),
    RadialInterestPolicy,
    MetricsEngineLoadPolicy, // CPUEngineLoadPolicy(0.8),
    PerEntityOrderedStateUpdateQos
  )

  private val bridgeSettings = Map[String, BridgeSettings](
    WORKER_TYPE -> FLOCKING_ENGINE_BRIDGE_SETTINGS
  )

  override def engineTypeToBridgeSettings(engineType: String, metadata: String): Option[BridgeSettings] = {
    bridgeSettings.get(engineType)
  }
}
