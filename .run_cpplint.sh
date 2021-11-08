set -ex

cpplint --recursive --quiet c/detype1
cpplint --recursive --quiet c/makeotf/include
cpplint --recursive --quiet c/makeotf/resource
cpplint --recursive --quiet c/makeotf/lib
cpplint --recursive --quiet c/makeotf/source
cpplint --recursive --quiet c/mergefonts
cpplint --recursive --quiet c/shared
cpplint --recursive --quiet c/rotatefont
cpplint --recursive --quiet c/sfntdiff
cpplint --recursive --quiet c/sfntedit
cpplint --recursive --quiet c/spot
cpplint --recursive --quiet c/tx
cpplint --recursive --quiet c/type1
