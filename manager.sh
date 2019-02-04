#!/bin/bash

show_help() {
  echo "manager: manager [-nd]"
  echo -e "\tCreate a new project that uses the Progression engine"
  echo ""
  echo -e "\tOptions:"
  echo -e "\t-n \tname of the project (must be a valid file name)"
  echo -e "\t-d \tpath to the project's root directory"
  echo -e ""
  echo -e "\tWill create a basic project with the name from -n in the directory from -d."
  echo -e "\tThe project will have a basic template and scene ready to be edited."
}

OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
projectName=""
projectRootDir=""
pwdCommand=""
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  pwdCommand="pwd"
else
  pwdCommand="pwd -W"
fi
progressionDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && $pwdCommand )"

while getopts ":n:d:h:" opt; do
  case $opt in
    n)
      projectName=$OPTARG
      ;;
    d)
      projectRootDir=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
    h)
      show_help
      exit 1
      ;;
  esac
done

if [[ "$projectName" = "" ]] || [[ "$projectRootDir" = "" ]]; then
  show_help
  exit 1
fi

mkdir -p $projectRootDir
currentDir=$(pwd)
cd $projectRootDir
projectRootDir="$($pwdCommand)"
cd $currentDir  
cp -r $progressionDir/templates/newproject/* $projectRootDir
mv $projectRootDir/code/newproject.hpp $projectRootDir/code/$projectName.hpp
mv $projectRootDir/code/newproject.cpp $projectRootDir/code/$projectName.cpp
mv $projectRootDir/configs/newproject.toml $projectRootDir/configs/$projectName.toml
find $projectRootDir -type f -print0 | xargs -0 sed -i 's|##projectName##|'$projectName'|g'
find $projectRootDir -type f -print0 | xargs -0 sed -i 's|##PROGRESSION_DIR##|'$progressionDir'|g'
find $projectRootDir -type f -print0 | xargs -0 sed -i 's|##projectRootDir##|'$projectRootDir'|g'
mkdir -p $projectRootDir/build
