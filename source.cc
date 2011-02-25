#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <linux/limits.h>
#include "buildgear/config.h"
#include "buildgear/options.h"
#include "buildgear/filesystem.h"
#include "buildgear/buildfiles.h"
#include "buildgear/dependency.h"
#include "buildgear/source.h"
#include "buildgear/download.h"

int CSource::Remote(string item)
{
   int i;
   string protocol[4] = { "http://",
                           "ftp://",
                         "https://",
                          "ftps://"  };
   
   for (i=0; i<4; i++)
   {
      if (item.find(protocol[i]) != item.npos)
         return true;
   }
   
   return false;
}

void CSource::Download(list<CBuildFile*> *buildfiles, string source_dir)
{
   CDownload Download;
   
   list<CBuildFile*>::iterator it;
   string command;
   
   /* Make sure that source dir exists */
   CreateDirectory(source_dir);

   /* Traverse buildfiles download list */
   for (it=buildfiles->begin(); it!=buildfiles->end(); it++)
   {
      istringstream iss((*it)->source);
      string item;
      
      // For each source item      
      while ( getline(iss, item, ' ') )
      {
         // Download item if it is a remote URL
         if (CSource::Remote(item))
            Download.URL(item, source_dir);
      }
   }
}

void CSource::Do(string action, CBuildFile* buildfile)
{
   string config;
   string command;
   stringstream pid;
   
   // Get PID
   pid << (int) getpid();
   
   // Set action
   config  = " ACTION=" + action;
   
   // Set required script variables
   config += " BUILD_FILES_CONFIG=" BUILD_FILES_CONFIG;
   config += " BUILD_TYPE=" + buildfile->type;
   config += " WORK_DIR=" WORK_DIR;
   config += " PACKAGE_DIR=" PACKAGE_DIR;
   config += " SOURCE_DIR=" + CSource::config->source_dir;
   config += " BUILD_LOG_FILE=" BUILD_LOG_FILE;
   config += " NAME=" + buildfile->name;
   config += " BG_PID=" + pid.str();
   config += " VERBOSE=no";
   
   command = config + " " SCRIPT " " + buildfile->filename;
   
   if (action == "build")
   {
      cout << "   Building      '" << buildfile->name << "'" << endl;
   }
   
   if (action == "add")
      cout << "   Adding        '" << buildfile->name << "'" << endl;
   
   if (action == "remove")
      cout << "   Removing      '" << buildfile->name << "'" << endl;
   
   if (system(command.c_str()) != 0)
   {
      cout << "Failed" << endl;
      exit(EXIT_FAILURE);
   }
}

bool CSource::UpToDate(CBuildFile *buildfile)
{
   string package;
   
   package = PACKAGE_DIR "/" +
             buildfile->name + "#" + 
             buildfile->version + "-" +
             buildfile->release + PACKAGE_EXTENSION;
   
   if (!FileExists(package))
      return false;
   
   if (Age(package) > Age(buildfile->filename))
      return true;
   
   return false;
}

bool CSource::DepBuildNeeded(CBuildFile *buildfile)
{
   list<CBuildFile*>::iterator it;
   
   for (it=buildfile->dependency.begin(); it!=buildfile->dependency.end(); it++)
   {
      if ((*it)->build)
         return true;
      
      if (DepBuildNeeded(*it))
         return true;
   }
   
   return false;
}

void CSource::Build(list<CBuildFile*> *buildfiles, CConfig *config)
{
   list<CBuildFile*>::iterator it;
   list<CBuildFile*>::reverse_iterator rit;
   int result;
   
   CSource::config = config;

   // Remove old build log
   result = system("rm -f " BUILD_LOG_FILE);

   // FIXME:
   // Check if buildfiles/config is newer than target package or target buildfiles
   // If so warn and delete work/packages (forces total rebuild)

   // Set build action of all builds (based on package vs. Buildfile age)
   for (it=buildfiles->begin(); it!=buildfiles->end(); it++)
   {
      if (!UpToDate((*it)))
         (*it)->build = true;
      
//      cout << "   " << (*it)->name << ":build = " << (*it)->build << endl;
   }

   // Set build action of all builds (based on dependencies build status)
   for (it=buildfiles->begin(); it!=buildfiles->end(); it++)
   {
      // Skip if build status already set
      if ((*it)->build)
         continue;
      
      // If one or more dependencies needs to be build
      if (DepBuildNeeded((*it)))
      {
         // Then build is needed
         (*it)->build = true;
      } else
      {
         // Else no build is needed
         (*it)->build = false;
      }
   }

/*
   // Show new build status
   cout << "New build status:" << endl;
   for (it=buildfiles->begin(); it!=buildfiles->end(); it++)
   {
      cout << "   " << (*it)->name << ":build = " << (*it)->build << endl;
   }
*/

   // Only build if selected build requires a build
   if (buildfiles->back()->build)
   {
      cout << endl;
      
      // Process build order
      for (it=buildfiles->begin(); it!=buildfiles->end(); it++)
      {
         // Only build if build (package) is not up to date
         if ((*it)->build == true)
            Do("build", (*it));
      
         // Don't add primary build
         if ((*it) != buildfiles->back())
            Do("add", (*it));
      }
   
      // Process build order in reverse
      for (rit=buildfiles->rbegin(); rit!=buildfiles->rend(); rit++)
      {
         // Remove all except primary build
         if ((*rit) != buildfiles->back())
            Do("remove", (*rit));
      }
   }

   // Delete work dir
   result = system("rm -rf " WORK_DIR);
}
