package improbable.apps

import com.typesafe.scalalogging.Logger
import improbable.papi.EntityId
import improbable.papi.world.AppWorld
import improbable.papi.world.messaging.CustomMsg
import improbable.papi.worldapp.WorldApp
import scala.concurrent.duration._

case class FlockerProfileEvent(entity:EntityId, period:Double, totalTime:Double, numSamples:Int) extends CustomMsg

class FlockingStats(world:AppWorld, logger:Logger) extends WorldApp {

  var theRecord = Map[EntityId, Double]()
  var numSamples = 0

  world.messaging.onReceive {
    case FlockerProfileEvent(e,p,t, n) =>
      theRecord = theRecord.updated(e, t+theRecord.getOrElse(e, 0.0))
      numSamples = numSamples+n
  }

  val samplePeriod = 3.0

  world.timing.every(samplePeriod.seconds) {


    val targetFPS = 8

    val totalFrameTime = theRecord.foldLeft(0.0) {
      (sum,r) =>
        sum + r._2
    }
    val targetFrameTime = 1.0/targetFPS
    val targetTotalFrameTime = targetFrameTime*theRecord.size


    val load = totalFrameTime/targetTotalFrameTime

    val timeStamp = System.nanoTime()
    logger.info("FlockStats [" + timeStamp + "] TotalIterations: " + numSamples + "; Load: "+ load)

    theRecord = Map.empty
    numSamples = 0
  }
}
