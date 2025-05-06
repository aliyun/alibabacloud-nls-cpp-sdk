#!/bin/bash -e

COMMIT_ID=$(git rev-parse --short=7 HEAD)

if [ -f version ]; then
    VERSION=$(cat version)
    VERSION=$(echo "$VERSION" | sed 's/-//g')
else
    echo "Version file not found."
    exit 1
fi

echo "VERSION: " ${VERSION}
echo "COMMIT_ID: " ${COMMIT_ID}
PACK_DIR_NAME="NlsCppSdk_Linux_${VERSION}_${COMMIT_ID}"
PACK_FILE_NAME="NlsCppSdk_Linux-x86_64_${VERSION}_${COMMIT_ID}.tar.gz"
echo "PACK_DIR_NAME: " ${PACK_DIR_NAME}
echo "PACK_FILE_NAME: " ${PACK_FILE_NAME}

git_root_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
install_folder=${git_root_path}/${PACK_DIR_NAME}
mkdir -p ${install_folder}
pack_file=${git_root_path}/${PACK_FILE_NAME}
build_install_folder=${git_root_path}/build/install
echo "install_folder: " ${install_folder}
echo "pack_file: " ${pack_file}
echo "build_install_folder: " ${build_install_folder}
echo "============================"

echo " building SDK with debug and CXX11 api 0 --->"
${git_root_path}/scripts/build_linux.sh all debug 0 > debug_0.log 2>&1
tar_file=$(find ${git_root_path}/build/install -maxdepth 1 -name '*.tar.gz' | head -n 1)
echo "  build done, get file " ${tar_file}
base_tar_file=$(basename "$tar_file")
echo "  get file name " ${base_tar_file}
BASENAME="${base_tar_file%.tar.gz}"
echo "  get file basename " ${BASENAME}
TIMESTAMP="${BASENAME##*NlsSdk3.X_LINUX_}"
echo "  get file timestamp " ${TIMESTAMP}
NEW_TIMESTAMP="${TIMESTAMP:0:-4}"
echo "  get file new timestamp " ${NEW_TIMESTAMP}
new_tar_file="NlsSdk3.X_LINUX_Debug_${NEW_TIMESTAMP}.tar.gz"
echo "  get file new tar file " ${new_tar_file}
echo "  cp -f " ${tar_file} "to" ${PACK_DIR_NAME}/${new_tar_file}
cp -f ${tar_file} ${PACK_DIR_NAME}/${new_tar_file}

echo " building SDK with release and CXX11 api 0 --->"
${git_root_path}/scripts/build_linux.sh all release 0 > release_0.log 2>&1
tar_file=$(find ${git_root_path}/build/install -maxdepth 1 -name '*.tar.gz' | head -n 1)
echo "  build done, get file " ${tar_file}
base_tar_file=$(basename "$tar_file")
echo "  get file name " ${base_tar_file}
BASENAME="${base_tar_file%.tar.gz}"
echo "  get file basename " ${BASENAME}
TIMESTAMP="${BASENAME##*NlsSdk3.X_LINUX_}"
echo "  get file timestamp " ${TIMESTAMP}
NEW_TIMESTAMP="${TIMESTAMP:0:-4}"
echo "  get file new timestamp " ${NEW_TIMESTAMP}
new_tar_file="NlsSdk3.X_LINUX_Release_${NEW_TIMESTAMP}.tar.gz"
echo "  get file new tar file " ${new_tar_file}
echo "  cp -f " ${tar_file} "to" ${PACK_DIR_NAME}/${new_tar_file}
cp -f ${tar_file} ${PACK_DIR_NAME}/${new_tar_file}

echo " building SDK with debug and CXX11 api 1 --->"
${git_root_path}/scripts/build_linux.sh all debug 1 > debug_1.log 2>&1
tar_file=$(find ${git_root_path}/build/install -maxdepth 1 -name '*.tar.gz' | head -n 1)
echo "  build done, get file " ${tar_file}
base_tar_file=$(basename "$tar_file")
echo "  get file name " ${base_tar_file}
BASENAME="${base_tar_file%.tar.gz}"
echo "  get file basename " ${BASENAME}
TIMESTAMP="${BASENAME##*NlsSdk3.X_LINUX_}"
echo "  get file timestamp " ${TIMESTAMP}
NEW_TIMESTAMP="${TIMESTAMP:0:-4}"
echo "  get file new timestamp " ${NEW_TIMESTAMP}
new_tar_file="NlsSdk3.X_LINUX_Debug_CXX11_${NEW_TIMESTAMP}.tar.gz"
echo "  get file new tar file " ${new_tar_file}
echo "  cp -f " ${tar_file} "to" ${PACK_DIR_NAME}/${new_tar_file}
cp -f ${tar_file} ${PACK_DIR_NAME}/${new_tar_file}

echo " building SDK with release and CXX11 api 1 --->"
${git_root_path}/scripts/build_linux.sh all release 1 > release_1.log 2>&1
tar_file=$(find ${git_root_path}/build/install -maxdepth 1 -name '*.tar.gz' | head -n 1)
echo "  build done, get file " ${tar_file}
base_tar_file=$(basename "$tar_file")
echo "  get file name " ${base_tar_file}
BASENAME="${base_tar_file%.tar.gz}"
echo "  get file basename " ${BASENAME}
TIMESTAMP="${BASENAME##*NlsSdk3.X_LINUX_}"
echo "  get file timestamp " ${TIMESTAMP}
NEW_TIMESTAMP="${TIMESTAMP:0:-4}"
echo "  get file new timestamp " ${NEW_TIMESTAMP}
new_tar_file="NlsSdk3.X_LINUX_Release_CXX11_${NEW_TIMESTAMP}.tar.gz"
echo "  get file new tar file " ${new_tar_file}
echo "  cp -f " ${tar_file} "to" ${PACK_DIR_NAME}/${new_tar_file}
cp -f ${tar_file} ${PACK_DIR_NAME}/${new_tar_file}

echo "============================"
echo " pack SDK ..."
echo "  tar -czf " ${pack_file} ${PACK_DIR_NAME}
tar -czf ${pack_file} ${PACK_DIR_NAME}
echo " pack SDK done."