rtp h264
rtp----------------
毋庸置疑   编码后的264包需要先把起始码去掉再给rtp去组包，接收端去掉rtp头和负载字节（0,1,2字节都有可能？）再补回起始码，这里可以用rtp发送接口的mark来标记哪个是4字节起始码

*rtp header 12Byte 混合流时16 可以扩展头   
字段负载类型h264时值为96

当负载类型为96时，rtp有三种负载结构
*rtp负载   通过负载第一个字节（F NRI TYPE）后5位判断负载格式
负载格式3种 1：单个NALU   2：聚合包 多个NALU合在一起  3：分片单元  一个NALU的一个slice  有FU-A FU-B 28 29 表示

单个NALU: 负载第一个字节 （F NRI TYPE）
rtp 负载第一个字节与 NALU一个字节重合，即rtp负载就是一个h264的NALU

分片NALU：负载前两字节 （FU indicator FU header）
FU indicator(F NRI Type)
FU header(S E R Type)  S开始 E结束 R保留 Type指示关键帧啥的

组合NALU：（F NRI TYPE）负载第一个字节值 24、25、26和27

完整rtp包  rtp头+（rtp扩展头）+（rtp负载头）+h264NALU+（补码）
（这里的h264NALU也许只是一部分   参考组合NALU）

h264 -------------------------
数据流是储存在介质上时，在每个NALU 前添加起始码
NALU单元  [start code]+[nalu header]+[nalu payload]
start code为4字节 0x00000001 代表该单元是SPS PPS 第一个切片
start code为3字节 0x000001  代表后继切片（slice）

默认编码方式Annex B
lib264x编码后得到的每个包是起始码+NALU（静态存储数据始终0x000001 ？） 发给ffmpeg解码的时候avcodec_decode_video需要起始码）
live555发送时要去掉起始码，也许客户端自己回回补吧
（Annex B to MP4 : Remove start codes, insert length of NAL, filter out SPS, PPS and AU delimiter.）

原始编码方式   AVCC
[SIZE (4 bytes)]--[NAL]--[SIZE (4 bytes)]--[NAL] etc
不带起始码 所以也不带描述信息 此时必须借助某个全局的数据来获得编码器的profile,level,PPS,SPS等信息才可以解码

rtsp------------------
rtsp协议通过SDP发送SPS PPS profile等信息

窗口消息队列/事件队列

排序

快排  
空间log(n) 最差O(n) 时间平均nlog(n)哨兵总在中间相遇  最坏n^2哨兵总在边上
哨兵选第一个   
记录哨兵值  
最右边先找小于哨兵的   若两个迭代器没相遇  则赋值给左边（左迭代器++）   最左边找大于等于哨兵的  若没相遇  赋值给右边（右迭代器++）
哨兵值赋给左迭代器
递归  左到左迭代器-1
递归  右到右迭代器+1

插入排序 时间 O (n^2)  1  最优 n 无移动 （递增有序）
较短的数组   第一个是有序   第二个插入第一个有序数组

堆排序  时间最坏O(nlgn)  空间O(1)  平均比快排慢
树

归并排序  稳定  需要而外空间log(n)
两两排序  合并

IO模型

IOCP
overlapped   
select   
epoll

GetQueuedCompletionStatus调用中设置超时

阻塞socket   非阻塞socket




--------------------------------------------
listen 都是空  未加入链表

进程  线程

内核态 用户态 

TCP/IP-----------
都是全双工   openssl单工
可靠性  ========================
TCP首部选项字节中还包含一个窗口扩大因子M，窗口字段的值左移M位
* 校验和	  12字节的伪首部 源IP地址、目的IP地址、保留字节(置0)、传输层协议号(TCP是6)、TCP报文长度(报头+数据)   首部校验   符合不一定准确 不符合一定有错  丢弃（TCP协议中规定，TCP的首部字段中有一个字段是校验和，发送方将伪首部、TCP首部、TCP数据使用累加和校验的方式计算出一个数字，然后存放在首部的校验和字段里，接收者收到TCP包后重复这个过程，然后将计算出的校验和和接收到的首部中的校验和比较，如果不一致则说明数据在传输过程中出错。这就是TCP的数据校验机制。 (A+B  接到时变成B+A校验通过但是错的）解决：应用层自己校验  MD5 使出错概率减少
* 序列号     有序 完整 去重  
* 确认应答    ACK+1  附带窗口大小
* 超时重发	   动态计算  linux以500ms整数倍
* 连接管理    三次握手 四次分手
* 流量控制    发送窗口   接收端在ACK附带窗口大小  当为0时发送端停止发送数据  定时发送探测数据  （主机对主机）
* 拥塞控制    慢启动  先指数增大到达阈值后加法增大  
	发送开始时，定义拥塞窗口为1.
	每收到一个ACK应答，拥塞窗口加1.
	每次发送数据时，拥塞窗口和发送窗口的较小的那一个决定了滑动窗口的大小。      （主机对网络）
	每次超时重发时，慢启动阈值变成原来的一半，同时拥塞窗口置为1 
性能提高=====================
* 滑动窗口  无需等待确认应答而可以继续发送数据的最大值，发送前4个段，不需要任何等待，直接发送。收到第一个ACK后，滑动窗口向后移动，继续发送第5个段（5001~6001）的数据，依次类推。

操作系统为了维护这一个滑动窗口，需要开辟缓冲区来记录当前还有哪些数据没有应答，只有确认过应答的数据，才能从缓冲区删了。这个缓冲区被称作“发送缓冲区”。

* 快速重传  漏掉一个包之后会不停重发 包含漏掉包序号的ACK  后面的会先缓存下来，发送端收到三次漏包ACK时，重发
* 延迟应答   立即应答的话窗口比较小，此时消费数据比较快的话就限制了滑动窗口大小，即网络吞吐量   延迟应答限制  1.数量限制：每隔N个包就应答一次（一般n=2） 2.时间限制：超过最大延迟时间就应答一次(一般200ms)。
* 捎带应答   确认信息被附在往外发送的数据帧上（使用帧头中的ask域）
* 面向字节流 即数据以BYTE为单位而不是bit 有发送缓冲  有接收缓冲  发送时太小会等一会儿  太大切片  接收时从网卡驱动到内核接收缓冲   

三个定时器==================
* 超时重传定时器  
* 保活定时器（每隔一段时间发送一次数据，收到响应，表示活着）。
* TIME_WAIT定时器  延迟应答？
粘包   http用空行分割报文和有效字段分隔开

UDP-----------------
校验和可选

---------------------
连接的缓存满了该如何？


stl容器

map------------插入迭代器不失效 
红黑树
性质1.节点是红色或黑色。
性质2.根节点是黑色。
性质3 每个叶节点（NIL节点，空节点）是黑色的。(空节点被省略了，其实黑节点下接着一个黑节点)
性质4 每个红色节点的两个子节点都是黑色。(从每个叶子到根的所有路径上不能有两个连续的红色节点)
性质5.从任一节点到其每个叶子的所有路径都包含相同数目的黑色节点。

优势：插入O(1) 删除 最多三次O(1)  最多多一次比较

AVL树
优势：找的快   插入O(1) 删除O(logN)  空间开销大  维护较慢

hash表  
将key值通过哈希函数得到一个固定长度的整型数  对数组长度取余作为下标    数组长度尽量不要接近2的整数幂  最好是奇数  因为如果是2的倍数  取余的结果总是0和奇数   偶数槽就浪费了

优势：插入 删除 查找 都是常数级别， 适合静态数据
劣势：key比较大时处理溢出和冲突（开发地址法，链地址法），内存占用大，

vector------------
数组   sort 快排  插入排序  堆排序  随机访问

dequeue------------
指针  数组  前后插入删除快

list------------
双向链表   插入迭代器不失效  无论哪里插入，删除都快

set------------
红黑树   有序  对比hash_set  集合的交集，并集等比较操作 有序的红黑树更有利
