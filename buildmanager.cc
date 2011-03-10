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
#include "buildgear/buildmanager.h"
#include "buildgear/download.h"

void CBuildManager::Do(string action, CBuildFile* buildfile)
{
   string config;
   string command;
   stringstream pid;
   
   // Get PID
   pid << (int) getpid();
   
   // Set script action
   config  = " BG_ACTION=" + action;
   
   // Set required script variables
   config += " BG_BUILD_FILES_CONFIG=" BUILD_FILES_CONFIG;
   config += " BG_BUILD_TYPE=" + buildfile->type;
   config += " BG_WORK_DIR=" WORK_DIR;
   config += " BG_PACKAGE_DIR=" PACKAGE_DIR;
   config += " BG_SOURCE_DIR=" SOURCE_DIR;
   config += " BG_SYSROOT_DIR=" SYSROOT_DIR;
   config += " BG_BUILD_LOG=" BUILD_LOG;
   config += " BG_PID=" + pid.str();
   config += " BG_VERBOSE=no";
   config += " BG_BUILD=" + Config.build_system;
   config += " BG_HOST=" + Config.host_system;

   // Apply build settings to all builds if '--all' is used
   if (Config.all)
   {
      config += " BG_UPDATE_CHECKSUM=" + Config.update_checksum;
      config += " BG_UPDATE_FOOTPRINT=" + Config.update_footprint;
      config += " BG_NO_STRIP=" + Config.no_strip;
      config += " BG_KEEP_WORK=" + Config.keep_work;
   } else
   {
      // Apply settings to main build
      if (Config.name == buildfile->name)
      {
         config += " BG_UPDATE_CHECKSUM=" + Config.update_checksum;
         config += " BG_UPDATE_FOOTPRINT=" + Config.update_footprint;
         config += " BG_NO_STRIP=" + Config.no_strip;
         config += " BG_KEEP_WORK=" + Config.keep_work;
      } else
      {
         // Apply default settings to the build dependants
         config += " BG_UPDATE_CHECKSUM=no";
         config += " BG_UPDATE_FOOTPRINT=no";
         config += " BG_NO_STRIP=no";
         config += " BG_KEEP_WORK=no";
      }
   }
   
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

bool CBuildManager::UpToDate(CBuildFile *buildfile)
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

bool CBuildManager::DepBuildNeeded(CBuildFile *buildfile)
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

void CBuildManager::Build(list<CBuildFile*> *buildfiles)
{
   list<CBuildFile*>::iterator it;
   list<CBuildFile*>::reverse_iterator rit;

   // FIXME:
   // Check if buildfiles/config is newer than package or buildfiles
   // If so warn and delete work/packages (force total rebuild)

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
      // Skip if build action already set
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

/*   
      // Process build order in reverse
      for (rit=buildfiles->rbegin(); rit!=buildfiles->rend(); rit++)
      {
         // Remove all except primary build
         if ((*rit) != buildfiles->back())
            Do("remove", (*rit));
      }
*/
   }
}

void CBuildManager::Clean(CBuildFile *buildfile)
{
   int result;
   string command;
   
   command  = "rm -f ";
   command += string(PACKAGE_DIR) + "/" + 
              buildfile->name + "#" +
              buildfile->version + "-" +
              buildfile->release + 
              PACKAGE_EXTENSION;
   
   result = system(command.c_str());
}

void CBuildManager::CleanAll(void)
{
   int result;
   
   result = system("rm -rf " PACKAGE_DIR);
}

void CBuildManager::CleanWork(void)
{
   int result;
   
   result = system("rm -rf " WORK_DIR);
}

void CBuildManager::CleanLog(void)
{
   int result;
   
   result = system("rm -f " BUILD_LOG);
}