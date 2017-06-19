
imdisk -a -t file -f uodos.img -o rem -m y:

mkdir y:\LEVEL1\
mkdir y:\LEVEL1\LEVEL2\
copy "c:\Users\Rich\Google Drive\University\Systems Programming\Assessment3_100327411\root\ROOTTEST.TXT" "y:\"
copy "c:\Users\Rich\Google Drive\University\Systems Programming\Assessment3_100327411\root\TEST.TXT" "y:\"
copy "c:\Users\Rich\Google Drive\University\Systems Programming\Assessment3_100327411\root\lower.txt" "y:\"
copy "c:\Users\Rich\Google Drive\University\Systems Programming\Assessment3_100327411\root\LEVEL1\LEVEL1.TXT" "y:\LEVEL1\"
copy "c:\Users\Rich\Google Drive\University\Systems Programming\Assessment3_100327411\root\LEVEL1\LEVEL2\LEVEL2.TXT" "y:\LEVEL1\LEVEL2\"

imdisk -D -m y: