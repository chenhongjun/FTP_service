#########################################################################
# File Name: push.sh
# Author: Willow
# mail: 1769003060@qq.com
# Created Time: 2016年11月23日 星期三 21时49分36秒
#########################################################################
#!/bin/bash
echo $1
git add .
git commit -m $1
git push -u origin master
