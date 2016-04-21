// This class provides common information about the SpatialOS application.

import sbt.Keys._
import sbt._
import Keys._
import scala.util.parsing.json.JSON

object SpatialOSBuild extends Build {

  private val projectManifestText = IO.read(file("../../spatialos.json")).trim
  private val projectManifestObj: Option[Any] = JSON.parseFull(projectManifestText)
  private val projectManifest: Map[String, Any] = projectManifestObj.get.asInstanceOf[Map[String, Any]]

  // The current version of the project, e.g. `0.1`
  val currentVersion = projectManifest.get("project_version").get.asInstanceOf[String]

  // The current version of the SpatialOS SDK that the project depends on, e.g. `5.0.0`
  val improbableVersion = scala.util.Properties.envOrNone("SPATIALOS_BUILD_NUMBER").getOrElse(projectManifest.get("sdk_version").get.asInstanceOf[String])

  // The name of the project
  val projectName = projectManifest.get("name").get.asInstanceOf[String]

  // A relative path to output artifacts to.
  val outputBaseDir = (file("..") / ".." / "build" / "assembly").toString

  // The root project object
  lazy val root = Project(id = projectName, base = file(".")).settings(
    scalaVersion := "2.11.7",
    version := currentVersion,
    libraryDependencies += "improbable" %% "deployment" % improbableVersion,
    unmanagedSourceDirectories.in(Compile) += baseDirectory.value / "generated",
    fork.in(runMain) := true,
    fork.in(Test) := true
  )
}
