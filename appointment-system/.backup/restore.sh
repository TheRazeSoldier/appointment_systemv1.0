#!/bin/bash
# 还原到之前的设计版本
echo "正在还原设计..."
cp /workspace/appointment-system/.backup/index.html.bak /workspace/appointment-system/frontend/index.html
cp /workspace/appointment-system/.backup/style.css.bak /workspace/appointment-system/frontend/css/style.css
cp /workspace/appointment-system/.backup/app.js.bak /workspace/appointment-system/frontend/js/app.js
echo "还原完成！重启服务器即可。"