根据test用例，封装自己的sftp控制台应用；
cli介绍：

1、put文件  
	-h xxxx -u xxx -pwd xxx -o put -s local_path\\file  -d remote_path

2、get文件
	-h xxxx -u xxx -pwd xxx -o get -s remote_path/file  -d local_path

3、delete文件
	-h xxxx -u xxx -pwd xxx -o del   -d remote_path/file

4、mkdir创建远端文件夹（父文件夹必须存在，否则失败）
	-h xxxx -u xxx -pwd xxx -o  mkdir   -d remote_path

5、rmdir删除远端文件夹（文件夹必须存在，否则失败）
	-h xxxx -u xxx -pwd xxx -o  rmdir   -d remote_path

6、upload目录（上传本地目录下所有文件，包括子文件夹）
	-h xxxx -u xxx -pwd xxx -o updir -s local_path -d remote_path

7、download目录(至下载远程路径下的文件，不包含子文件夹)
	-h xxxx -u xxx -pwd xxx -o downdir -s remote_path -d local_path