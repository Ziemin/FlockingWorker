package improbable.apps

import java.util.function.Consumer

import com.typesafe.scalalogging.Logger
import improbable.papi.world.AppWorld
import improbable.papi.worldapp.{WorldApp, WorldAppLifecycle}
import org.flagz.{FlagContainer, FlagInfo, ScalaFlagz}

import scala.concurrent.duration._


class SimulationReloader(val world: AppWorld, val logger: Logger, val lifecycle: WorldAppLifecycle) extends WorldApp {

  val respawner = new Respawner(world)
  val path = "./src/main/resources/parameters"

  SimulationReloader.restart.withListener(
    new Consumer[Integer] {
      override def accept(t: Integer): Unit = {
        respawner.restart()
      }
    }
  )

  SimulationReloader.reloadParameters(path)

  import java.nio.file._
  try {
    val f = Paths.get(".").getFileName

    val file = Paths.get(path)
    val watcher = FileSystems.getDefault.newWatchService

    file.register(
      watcher,
      StandardWatchEventKinds.ENTRY_MODIFY
    )
    world.timing.every(1.second) {
      val key = watcher.poll

      if (key != null && key.pollEvents().size() > 0) {
        SimulationReloader.reloadParameters(path)
        key.reset()
      }
    }
  }
  catch {
    case throwable: Throwable =>
      logger.error("failure to reload the simulation")
      respawner.restart()
  }
}


object SimulationReloader extends FlagContainer {

  def reloadParameters(path:String): Unit = {
    try {
      val params = scala.io.Source.fromFile(path+"/parameters.txt").getLines().toArray.map(_.trim).filterNot(_.isEmpty)
      println("\nReloading parameters: ")
      params.foreach(println)
      ScalaFlagz.parse(params, List("templates","improbable.apps"))
    } catch {
      case throwable: Throwable =>
    }
  }

  @FlagInfo(help = "change this to reload the simulation")
  val restart = ScalaFlagz.valueOf(0)

}
