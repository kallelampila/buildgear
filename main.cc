#include <string>
#include <cstdlib>
#include "buildgear/config.h"
#include "buildgear/configfile.h"
#include "buildgear/debug.h"
#include "buildgear/clock.h"
#include "buildgear/options.h"
#include "buildgear/filesystem.h"
#include "buildgear/buildfiles.h"
#include "buildgear/dependency.h"
#include "buildgear/source.h"
#include "buildgear/download.h"
#include "buildgear/tools.h"

Debug debug(cout);
CConfig      Config;

int main (int argc, char *argv[])
{
   COptions     Options;
   CConfigFile  ConfigFile;
   CClock       Clock;
   CFileSystem  FileSystem;
   CBuildFiles  BuildFiles;
   CDependency  Dependency;
   CSource      Source;
   CTools       Tools;

   /* Debug stream option */
   debug.On() = false;

   /* Start counting elapsed time */
   Clock.Start();
   
   /* Disable cursor */
//   cout << TERMINFO_CIVIS;
   
   /* Parse command line options */
   Options.Parse(argc, argv, &Config);
   
   /* Display help hint on incorrect download command */
   if ((Config.download) && (Config.name == "") && (Config.all==false))
   {
      cout << "Please specify build name or use 'download --all' to download source files of all builds\n";
      exit(EXIT_FAILURE);
   }
   
   /* Display help hint on incorrect build command */
   if ((Config.build) && (Config.name == ""))
   {
      cout << "Please specify build name\n";
      exit(EXIT_FAILURE);
   }
   
   /* Display help hint on incorrect clean command */
   if ((Config.clean) && (Config.name == "") && (Config.all==false))
   {
      cout << "Please specify build name or use 'clean --all' to clean all builds\n";
      exit(EXIT_FAILURE);
   }

   /* Show buildfiles help*/
   if (Config.show)
   {
      if (Config.help)
         Source.ShowBuildHelp();
         exit(EXIT_SUCCESS);
   }

   /* Search for build root directory */
   FileSystem.FindRoot(ROOT_DIR);

   /* Guess host and build */
   Config.GuessSystem();

   /* Parse buildgear configuration file(s) */
   ConfigFile.Parse(GLOBAL_CONFIG_FILE, &Config);
   ConfigFile.Parse(LOCAL_CONFIG_FILE, &Config);

   /* Parse buildfiles configuration file */
   ConfigFile.Parse(BUILD_FILES_CONFIG, &Config);
   
   /* Correct source dir */
   Config.CorrectSourceDir();

   /* Correct name */
   Config.CorrectName();

   /* Find host and target build files */
   cout << "\nSearching for build files..     ";
   FileSystem.FindFiles(BUILD_FILES_DIR,
                        BUILD_FILE,
                        &BuildFiles.buildfiles);

   /* Print number of buildfiles found */
   cout << BuildFiles.buildfiles.size() << " files found\n";

   /* Parse and verify buildfiles */
   cout << "Loading build files..           ";
   BuildFiles.ParseAndVerify(&BuildFiles.buildfiles);
   
   /* Show buildfiles meta info (debug only) */
//   BuildFiles.ShowMeta(&BuildFiles.buildfiles);
   
   /* Load dependencies */

   BuildFiles.LoadDependency(&BuildFiles.buildfiles);
   cout << "Done\n";

   if (!Config.download)
   {
      /* Resolve dependencies */
      cout << "Resolving dependencies..        ";
      if (Config.host_toolchain != "")
         Dependency.Resolve(Config.host_toolchain, &BuildFiles.buildfiles);
      if (Config.target_toolchain != "")
         Dependency.Resolve(Config.target_toolchain, &BuildFiles.buildfiles);
      Dependency.Resolve(Config.name, &BuildFiles.buildfiles);
      cout << "Done\n";
   }
   
   /* Handle show options */
   if (Config.show)
   {
      if (Config.download_order)
         Dependency.ShowDownloadOrder();
      
      if (Config.build_order)
         Dependency.ShowBuildOrder();
         
      cout << endl;
      
      exit(EXIT_SUCCESS);
   }
   
   /* Create build directory */
   FileSystem.CreateDirectory(BUILD_DIR);

   if ((Config.download) && (Config.all))
      Dependency.download_order=BuildFiles.buildfiles;

   /* Download source files */
   cout << "Downloading sources..           ";
   Source.Download(&Dependency.download_order, Config.source_dir);
   cout << "Done\n";

   /* Quit if download command */
   if (Config.download)
   {
      cout << endl;
      exit(EXIT_SUCCESS);
   }
   
   /* Check for required preinstalled host tools */
   cout << "Checking host tools and libs..  " << flush;
   Tools.Check();
   Tools.RunToolsFile();
   cout << "Done\n\n";
   
   /* Show system information */
   Config.ShowSystem();
   
   /* Start building */
   cout << "Building '" << Config.name << "'.. ";
   if (Config.build)
      Source.Build(&Dependency.build_order, &Config);
   cout << "Done\n\n";

   /* Stop counting elapsed time */
   Clock.Stop();
   
   /* Show elapsed time */
   Clock.ShowElapsedTime();
   
   /* Show log location */
   cout << "See " BUILD_LOG " for details.\n\n";
   
   /* Enable cursor again */
   cout << TERMINFO_CNORM;
}
