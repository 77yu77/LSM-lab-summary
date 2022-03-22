# LSM-lab-summary
针对大二课程所发布的大作业LTM KV进行个人的回顾与总结
## 背景介绍
十多年前，谷歌发布了 BigTable（2006 年）的论文，为开源界在大数据领域带来了无数的灵感，其中在“BigTable”的论文中很多很酷的方面之一就是它所使用的文件组织方式，这个方法更一般的名字叫 Log Structured-Merge Tree。在面对亿级别之上的海量数据的存储和检索的场景下，我们选择的数据库通常都是各种强力的 NoSQL，比如 Hbase，Cassandra，Leveldb，RocksDB 等等，而这些强大的 NoSQL 数据库都有一个共性，就是其底层使用的数据结构，都是仿照“BigTable”中的文件组织方式来实现的，也就是LSM-Tree。在这个lab中，我所实现的LSM-Tree 是一种支持大量写操作的数据结构，其通过 Memtable 来缓存数据，在 Memtable 满时写入硬盘，通过多层级结构来实现硬盘的存储，从而提高数据写入的效率。
## 实现结构
LSM tree键值存储系统分为内存存储和硬盘存储两部分。<br>
内存存储结构上使用一个MemTable，MemTable通过跳表保存键值对。跳表的查找实现是从指定的首节点出发，向右查找，如果后面出现更大的点，则跳入下层，继续向右查找，不断迭代，直到命中或者穿透底层查找失败，这样查找通过数学运算是O(logn)，插入和删除都是在查找的基础上进行修改。<br>
考虑到在实际情况下，存在内存的Memtable可能会由于断电导致数据丢失，所以增加应该ahead-log，在硬盘里保存一份当前状态的Memtable,在每次操作内存前先对硬盘的Memtable进行操作。<br>
其次就是硬盘存储的设计，采用分层存储的方式来存储，每一层包括多个文件，每个文件称为SSTable,用来有序存储多个键值对，每个SSTable包括header、Bloom Filter、索引区和数据区。Header存放的是元数据，包括时间戳、键值对数量、键最小值和最大值。 <br>
![LSM结构图](https://github.com/77yu77/LSM-lab-summary/blob/main/picture/LSM%E7%BB%93%E6%9E%84%E5%9B%BE.jpg "结构图")<br>
在硬盘存储上，会保证除Level 0 外，其他Level的各个SSTable的键值区间不相交，限制MemTable和SSTable的大小相同，结构相同，当SSTable超过一定大小（2MB）,就会往下push一个SSTable，放在Level 0，每一层Level 会有个数限制，如Level 0 两个，后面乘2，当Level 0 超过数量时，会将当前层所有SSTable放到下一层进行合并，除了Level 0 之外，其他Level 满了时，取出超出文件的数量的SSTable，优先选择时间戳最小的文件放到下一层进行合并，若没有下一层则新建一层，在当前层的合并操作是统计新插入的所有SSTable的范围，取出当前层所有与该范围有交集的SSTable进行合并，将上述所有文件通过归并排序的方式进行合并（两个有序数对合并），重新生成新的一组SSTable。
## 性能测试（每组数据都是测量10次取平均值）
### 测量 Get、Put、Delete 操作的延迟<br>
 ![data1](https://github.com/77yu77/LSM-lab-summary/blob/main/picture/data1.jpg "data1")<br>
当数据长度不变时，Put 和 Delete 操作的延迟随着数据数量的增加而增加，Put 增加幅度更大，这是因为 Put 调用 compaction 的次数比Delete 要多得多，平均下来时间增大更多，而 Get 操作刚开始增加，后面保持平缓，是因为前面直接在 Memtable 里面读，时间较快，后面必须在磁盘读，而读操作集中在内存查找，最后才到硬盘上查找，时间影响不大，故趋于平缓。<br>
### 索引缓存与 Bloom Filter 的效果测试<br>
针对索引缓存和 Bloom Filter 的优化，在GET操作下测量其性能表现
1. 内存中没有缓存 SSTable 的任何信息，从磁盘中访问 SSTable 的索引，在找到 offset 之后读取数据<br>
2. 内存中只缓存了 SSTable 的索引信息，通过二分查找从 SSTable 的索引中找到 offset，并在磁盘中读取对应的值<br>
3. 内存中缓存 SSTable 的 Bloom Filter 和索引，先通过 Bloom Filter判断一个键值是否可能在一个 SSTable 中，如果存在再利用二分查找，否则直接查看下一个 SSTable 的索引<br>
数据如下：<br>
 ![data3](https://github.com/77yu77/LSM-lab-summary/blob/main/picture/data3.jpg "data3")<br>
 通过不在内存存 SSTable，采用二分法于使用 Bloom Filter 三者对比，可以看出在磁盘读 offset 时 Get 时间非常慢，而将 SSTable 存在内存以及二分法效率比较高，性能提升非常大，采用 Bloom Filter 在一定程度上提高 Get 速度,而在这里不明显的原因可能是数据量不够大，还有就是SSTable比较小，只有2MB,而测试时每个value接近1KB，导致存的数据量比较小，而且SSTable的数量较少也会导致布隆过滤器效果不是很明显，所以二分查找会比较快。
## 调研LevelDB的实现
### log日志
leveldb的写操作并不是直接写入磁盘的，而是首先写入到内存。假设写入到内存的数据还未来得及持久化，leveldb进程发生了异常，抑或是宿主机器发生了宕机，会造成用户的写入发生丢失。因此leveldb在写内存之前会首先将所有的写操作写到日志文件中，也就是log文件。当 log文件大小超过限定值时，就定时做check point。Leveldb会生成新的Log文件和Memtable，后台调度会将Immutable Memtable的数据导出到磁盘，形成一个新的SSTable文件。<br>
### SSTable方面
LevelDB的SST文件由若干个4K大小的blocks组成，block也是读/写操作的最小单元，这样有利于读写操作；<br>
SST文件的最后一个block是一个index，指向每个data block的起始位置，以及每个block第一个entry的key值（block内的key有序存储）,而我则是放在起始位置，放在前面查找更方便。<br>
同一个block内的key可以共享前缀（只存储一次），这样每个key只要存储自己唯一的后缀就行了。如果block中只有部分key需要共享前缀，在这部分key与其它key之间插入"reset"标识。(猜想这是因为同一个block的key相关性比较强，很多会出现相同的前缀，比如按顺序存储的话一般只有后几位改变)<br>
### cache
读取操作如果没有在内存的memtable中找到记录，要多次进行磁盘访问操作,所以LevelDb中引入了两个不同的LRUCache:Table Cache和Block Cache.<br>
如果levelDb确定了key在某个level下某个文件A的key range范围内,那么levelDb会首先查找Table Cache，看这个文件是否在缓存里，没有再去打开SSTable文件，并将其index部分读入内存，然后插入Cache里面，去index里面定位哪个block包含这个Key.<br>
Block Cache是为了加快这个过程的，其中的key是文件的cache_id加上这个block在文件中的起始位置block_offset。而value则是这个Block的内容，如果levelDb发现这个block在block cache中，那么可以避免读取数据，直接在cache里的block内容里面查找key的value就行，如果没找到，那么读入block内容并把它插入block cache中.<br>
具体对Cache进行分析：<br>
leveldb中的Cache主要用到了双向链表、哈希表和LRU（least recently used）思想。<br>
LRUHandle表示了Cache中的每一个元素，通过指针形成一个双向循环链表,LRUHandle 结构将hash值相同的所有元素串联成一个双向循环链表，通过指针next_hash来解决hash 碰撞.<br>
leveldb通过HandleTable维护一个哈希表,哈希表中包含对LRUHandle的查询、插入与删除。<br>
LRUCache顾名思义是指一个缓存，同时它用到了LRU的思想,LRUCache维护了一个双向循环链表lru_和一个hash表table,当要插入一个元素时，首先将其插入到链表lru的尾部，然后根据hash值将其插入到hash表中。当hash表中已存在hash值与要插入元素的hash值相同的元素时，将原有元素从链表中移除，这样就可以保证最近使用的元素在链表的最尾部，这也意味着最近最少使用的元素在链表的头部，这样即可实现LRU的思想。
## 从LevelDB中的收获与优化
### 添加写操作的log日志
当每次发生写操作时，先将操作写到log文件进行持久化，新启一个线程对log文件进行大小检测，当log文件超过2MB时进行快照生成，独立的一个快照文件。此时有个问题就是当生成快照期间可能会对log文件进行写入，导致出现错误，这边使用了互斥锁，写log操作和生成快照申请互斥锁（lock以及）。
### SSTable优化
项目的SSTable是在前面存放header，布隆过滤器和索引offset，这样会导致一个问题就是当确定索引offset时，由于我是连续空间的存储，取出来也是数组取出，那在合并产生新的SSTable的时候，归并排序一次产生一个键值对，无法立刻生成索引，因为如果直接放在数组里面，每次插入新的键值会使得所有已存放键值的offset向后移，所以得等达到2MB大小的键值及元数据后才能确定元数据大小，进而确定offset，产生多余的操作，而且在查找数据的时候也需要加上元数据长度这一段。
###  cache优化
通过链表做了定长度的LRU cache，再通过hash表存对应节点的位置以及key值（方便匹配），hash表采用链表哈希实现。每次满长度就将链表头删除，插入链表尾，所以要记录链表头与尾。

## RocksDB的实现与优化
### 增加了column family，有了列簇的概念，可把一些相关的key存储在一起
每个column familyl的memtable与sstable都是分开的，所以每一个column family都可以单独配置，所有column family共用同一个WAL log文件，可以保证跨column family写入时的原子性.Column Family主要是提供给RocksDB一个逻辑的分区.Column Families 背后的主要思想是它们WAL log文件，而不共享内存表和表文件。通过共享WAL log文件，我们获得了原子写入的巨大好处。通过分离 memtables 和 table 文件，我们可以独立配置列族并快速删除它们。
### 内存中 max_write_buffer_number的设置
在刷新到 SST 文件之前，内存中建立的最大内存表数，默认为2，
### 支持并发插入
### HashSkiplist MemTable
HashSkipList 将数据组织在一个哈希表中，每个哈希桶作为一个Skiplist，而 HashLinkList 将数据组织在一个哈希表中，每个哈希桶作为一个排序的单链表。这两种类型都是为了减少查询时的比较次数而构建的。<br>

参考：<br>
https://github.com/facebook/rocksdb
