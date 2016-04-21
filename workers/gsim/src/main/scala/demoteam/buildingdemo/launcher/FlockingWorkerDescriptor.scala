package demoteam.buildingdemo.launcher

import java.io.File

import improbable.{OperatingSystem, OperatingSystemUtil}
import improbable.fapi.engine.{EngineStartConfig, DownloadableEngineDescriptor}
import java.nio.file.Path

class FlockingWorkerDescriptor extends DownloadableEngineDescriptor {
  def enginePlatform = FlockingWorkerBridgeSettings.WORKER_TYPE

  override def startCommand(config: EngineStartConfig, enginePath: Path) : Seq[String] = {
    Seq(
      makeExecutablePath(enginePath).toString,
      config.receptionistIp,
      "7777",
      config.engineId
    )
  }

  private def ensureFileIsExecutable(file: File): Unit = {
    file.setExecutable(true)
    file.setReadable(true)
  }

  private def makeExecutablePath(startPath:Path) : Path = {
    val operatingSystem = OperatingSystemUtil.currentOperatingSystem
    val executableName = s"$enginePlatform@$operatingSystem"
    val localExecutablePath = operatingSystem match {
      case OperatingSystem.Linux =>
        executableName

      case OperatingSystem.Mac =>
        s"$executableName.app/Contents/MacOS/$executableName"

      case OperatingSystem.Windows =>
        s"$executableName.exe"
    }
    val absoluteExecutablePath = startPath.resolve(localExecutablePath).toAbsolutePath

    ensureFileIsExecutable(absoluteExecutablePath.toFile)
    absoluteExecutablePath
  }
}

object FlockingWorkerDescriptor extends FlockingWorkerDescriptor
