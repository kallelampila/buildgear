#include <string>
#include <cstdlib>
#include "buildgear/config.h"
#include "buildgear/configfile.h"
#include "buildgear/debug.h"
#include "buildgear/time.h"
#include "buildgear/options.h"
#include "buildgear/filesystem.h"
#include "buildgear/buildfiles.h"
#include "buildgear/dependency.h"
#include "buildgear/source.h"
#include "buildgear/download.h"
#include "buildgear/tools.h"

Debug debug(cout);

int main (int argc, char *argv[])
{
   CConfig      Config;
   COptions     Options;
   CConfigFile  ConfigFile;
   CTime        Time;
   CFileSystem  FileSystem;
   CBuildFiles  BuildFiles;
   CDependency  Dependency;
   CSource      Source;
   CTools       Tools;

   /* Start counting elapsed time */
   Time.Start();
   
   /* Disable cursor */
//   cout << TERMINFO_CIVIS;
   
   /* Parse command line options */
   Options.Parse(argc, argv, &Config);
   
   /* Debug stream option */
   debug.On() = false;

   /* Search for build root directory */
   FileSystem.FindRoot(ROOT_DIR);

   /* Guess host and build */
   Config.GuessSystem();

   /* Parse buildgear configuration file(s) */
   ConfigFile.Parse(GLOBAL_CONFIG_FILE, &Config);
   ConfigFile.Parse(LOCAL_CONFIG_FILE, &Config);

   /* Parse buildfiles configuration file */
   ConfigFile.Parse(BUILD_CONFIG_FILE, &Config);
   
   /* Correct source dir */
   Config.CorrectSourceDir();

   /* Correct name */
   Config.CorrectName();

   /* Check for tools required by buildgear */
   Tools.Check();

   /* Future optimization
    * 
    * Track and load dependent buildfiles
    *  -> does not load all files
    *  -> minimizes I/O load
    */

   /* Find host and target build files */
   cout << "Searching for build files.. ";
   FileSystem.FindFiles(BUILD_FILES_DIR,
                        BUILD_FILE,
                        &BuildFiles.buildfiles);

   /* Print number of buildfiles found */
   cout << BuildFiles.buildfiles.size() << " files found\n\n";

   /* Parse and verify buildfiles */
   cout << "Loading build files.. ";
   BuildFiles.ParseAndVerify(&BuildFiles.buildfiles);
   
   /* Show buildfiles meta info (debug only) */
//   BuildFiles.ShowMeta(&BuildFiles.buildfiles);
   
   /* Load dependencies */

   BuildFiles.LoadDependency(&BuildFiles.buildfiles);
   cout << "Done\n\n";

   /* Note: 
    * Allowed dependency relations: TARGET -> TARGET
    *                               TARGET -> HOST
    *                                 HOST -> HOST
    */

   /* Resolve dependencies */
   cout << "Resolving dependencies.. ";
   Dependency.Resolve(Config.target_toolchain, &BuildFiles.buildfiles);
   Dependency.Resolve(Config.name, &BuildFiles.buildfiles);
   cout << "Done" << endl << endl;

   /* Print resolved */
   Dependency.ShowResolved();

   /* Create build directory */
   FileSystem.CreateDirectory(BUILD_DIR);

   /* Download source files */
   cout << "Downloading sources.. ";
   Source.Download(&Dependency.download_order, Config.source_dir);
   cout << "Done\n\n";

   /* Quit if download command */
   if (Config.download)
      exit(EXIT_SUCCESS);
   
   /* Show system information */
   Config.ShowSystem();
   
   /* Start building */
   cout << "Building '" << Config.name << "'.. ";
   if (Config.build)
      Source.Build(&Dependency.build_order, &Config);
   cout << "Done" << endl << endl;

   /* Stop counting elapsed time */
   Time.Stop();
   
   /* Show elapsed time */
   Time.ShowElapsedTime();
   
   /* Show log location */
   cout << "See " BUILD_LOG_FILE " for details." << endl << endl;
   
   /* Enable cursor again */
   cout << TERMINFO_CNORM;
}
