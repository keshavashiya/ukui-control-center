rcdir   = $(datadir)/themes/ukui-theme/gtk-2.0/apps
rc_DATA = ukuicc.rc

icondir = $(pkgdatadir)/icons
facedir = $(datadir)/pixmaps/faces
icon_DATA = 打印机.png 电源管理.png 个性化.png 键盘.png 开机启动.png 默认应用程序.png 日期和时间.png 声音.png 鼠标.png 网络代理.png 网络连接.png 显示器.png 用户账号.png 系统检测.png
face_DATA = stock_person.png
SUBDIRS = color
EXTRA_DIST = $(rc_DATA) $(icon_DATA) $(face_DATA)

clean-local:
	rm -rf Makefile.in
