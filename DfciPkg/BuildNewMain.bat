

git clone https://github.com/microsoft/mu_plus.git mu_plus_git_filtered

cd mu_plus_git_filtered

git filter-repo^
  --path DfciPkg^
  --path ZeroTouchPkg/^
  --path XmlSupportPkg/^
  --path LICENSE.txt^
  --path pull_request_template.md^
  --path pip-requirements.txt^
  --path .gitignore^
  --path MsCorePkg/Include/Library/PasswordStoreLib.h^
  --path MsCorePkg/Library/PasswordStoreLibNull/^
  --path MsCorePkg/Include/Library/JsonLiteParser.h^
  --path MsCorePkg/Library/JsonLiteParser/^
  --path MsCorePkg/UnitTests/JsonTest/^
  --path-rename MsCorePkg/Include/Library/PasswordStoreLib.h:DfciPkg/Include/Library/PasswordStoreLib.h^
  --path-rename MsCorePkg/Library/PasswordStoreLibNull/:DfciPkg/Library/PasswordStoreLibNull/^
  --path-rename MsCorePkg/Include/Library/JsonLiteParser.h:DfciPkg/Include/Library/JsonLiteParser.h^
  --path-rename MsCorePkg/Library/JsonLiteParser/:DfciPkg/Library/JsonLiteParser/^
  --path-rename MsCorePkg/UnitTests/JsonTest/:DfciPkg/UnitTests/JsonTest/^
  --replace-text e:\ReplaceText.txt

xcopy \Src\FilesForDfciFeature\* . /s

git add *

git commit -m "Add files to support the Repo and CI Build"

git remote add git https://github.com/microsoft/mu_plus.git

git fetch git

git cherry-pick 520f16237fdf5f4ad427ce1beed2e83fb8c3c4fd

git cherry-pick e78812aa91e00cf58a9c06ba6c85709a7c42d6ef

git cherry-pick 50a049022d31cf1b4d945b607f4c68a768d3447b

git cherry-pick df39f374059f915ee0dc12a82c34f9fcc9fafcf8

git cherry-pick 80e5d0fd13068aefd4a20dadd136abfc09a2c615

git remote remove git

rem git push https://github.com/microsoft/mu_feature_dfci.git +release/202208:main
