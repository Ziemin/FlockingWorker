package behaviours.agent

import demoteam.{UpdateTimeEventData, FlockStatsWriter}
import improbable.apps.{FlockingStats, FlockerProfileEvent}
import improbable.papi.entity.behaviour.EntityBehaviourInterface
import improbable.papi.entity.{Entity, EntityBehaviour}
import improbable.papi.world.World
import scala.concurrent.duration._

trait FlockStatsInterface extends EntityBehaviourInterface
{
  def addStat(executionTime:Double) : Unit
}

class FlockStatsBehaviour(entity:Entity, world:World, flockStatsWriter: FlockStatsWriter) extends EntityBehaviour with FlockStatsInterface {

  override def addStat(executionTime:Double) : Unit = {

    flockStatsWriter.update.entityUpdateTime(flockStatsWriter.entityUpdateTime :+ UpdateTimeEventData(entity.entityId, 0.0, executionTime)).finishAndSend()
  }

  override def onReady() : Unit = {

    val samplePeriod = 3.0

    world.timing.every(samplePeriod.seconds) {

      if(flockStatsWriter.entityUpdateTime.nonEmpty) {
        val totalTime = flockStatsWriter.entityUpdateTime.foldLeft(0.0) {
          (a, b) =>
            a + b.duration
        }

        world.messaging.sendToApp(classOf[FlockingStats].getName, FlockerProfileEvent(entity.entityId, samplePeriod, totalTime, flockStatsWriter.entityUpdateTime.size))

        flockStatsWriter.update.entityUpdateTime(Nil).finishAndSend()
      }
    }
  }
}
