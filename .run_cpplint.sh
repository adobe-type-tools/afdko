set -ex

cpplint --exclude c/addfeatures/pstoken --recursive --quiet c/addfeatures
cpplint --recursive --quiet c/detype1
# Skip pstoken and typecomp for now
cpplint --recursive --quiet c/mergefonts
cpplint --recursive --quiet c/shared
cpplint --recursive --quiet c/rotatefont
cpplint --recursive --quiet c/sfntdiff
cpplint --recursive --quiet c/sfntedit
cpplint --recursive --quiet c/spot
cpplint --recursive --quiet c/type1
