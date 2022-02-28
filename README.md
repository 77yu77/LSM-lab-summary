# LSM-lab-summary
针对大二课程所发布的大作业LTM KV进行个人的回顾与总结
## 背景介绍
十多年前，谷歌发布了 BigTable（2006 年）的论文，为开源界在大数据领域带来了无数的灵感，其中在“BigTable”的论文中很多很酷的方面之一就是它所使用的文件组织方式，这个方法更一般的名字叫 Log Structured-Merge Tree。在面对亿级别之上的海量数据的存储和检索的场景下，我们选择的数据库通常都是各种强力的 NoSQL，比如 Hbase，Cassandra，Leveldb，RocksDB 等等，而这些强大的 NoSQL 数据库都有一个共性，就是其底层使用的数据结构，都是仿照“BigTable”中的文件组织方式来实现的，也就是LSM-Tree。在这个lab中，我所实现的LSM-Tree 是一种支持大量写操作的数据结构，其通过 Memtable 来缓存数据，在 Memtable 满时写入硬盘，通过多层级结构来实现硬盘的存储，从而提高数据写入的效率。
## 实现结构
LSM tree键值存储系统分为内存存储和硬盘存储两部分。<br>
内存存储结构上使用一个MemTable，MemTable通过跳表保存键值对。跳表的查找实现是从指定的首节点出发，向右查找，如果后面出现更大的点，则跳入下层，继续向右查找，不断迭代，直到命中或者穿透底层查找失败，这样查找通过数学运算是O(logn)，插入和删除都是在查找的基础上进行修改。<br>
考虑到在实际情况下，存在内存的Memtable可能会由于断电导致数据丢失，所以增加应该ahead-log，在硬盘里保存一份当前状态的Memtable,在每次操作内存前先对硬盘的Memtable进行操作。<br>
其次就是硬盘存储的设计，采用分层存储的方式来存储，每一层包括多个文件，每个文件称为SSTable,用来有序存储多个键值对，每个SSTable包括header、Bloom Filter、索引区和数据区。Header存放的是元数据，包括时间戳、键值对数量、键最小值和最大值。 <br>
![LSM结构图](https://github.com/77yu77/LSM-lab-summary/blob/main/picture/LSM%E7%BB%93%E6%9E%84%E5%9B%BE.jpg "结构图")
在硬盘存储上，会保证除Level 0 外，其他Level的各个SSTable的键值区间不相交，限制MemTable和SSTable的大小相同，结构相同，当SSTable超过一定大小（2MB）,就会往下push一个SSTable，放在Level 0，每一层Level 会有个数限制，如Level 0 两个，后面乘2，当Level 0 超过数量时，会将当前层所有SSTable放到下一层进行合并，除了Level 0 之外，其他Level 满了时，取出超出文件的数量的SSTable，优先选择时间戳最小的文件放到下一层进行合并，若没有下一层则新建一层，在当前层的合并操作是统计新插入的所有SSTable的范围，取出当前层所有与该范围有交集的SSTable进行合并，将上述所有文件通过归并排序的方式进行合并（两个有序数对合并），重新生成新的一组SSTable。
## 性能测试
