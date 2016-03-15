input=${1}
output=${2}.iconset
mkdir -p ${output}
sips -z 16 16     ${input} --out ${output}/icon_16x16.png
sips -z 32 32     ${input} --out ${output}/icon_16x16@2x.png
sips -z 32 32     ${input} --out ${output}/icon_32x32.png
sips -z 64 64     ${input} --out ${output}/icon_32x32@2x.png
sips -z 128 128   ${input} --out ${output}/icon_128x128.png
sips -z 256 256   ${input} --out ${output}/icon_128x128@2x.png
sips -z 256 256   ${input} --out ${output}/icon_256x256.png
sips -z 512 512   ${input} --out ${output}/icon_256x256@2x.png
sips -z 512 512   ${input} --out ${output}/icon_512x512.png
sips -z 1024 1024   ${input} --out ${output}/icon_512x512@2x.png
iconutil -c icns ${output}
rm -R ${output}
