
## NOTE:   
This project is deprecated and will not be mantained anymore. If you are interested in the HTTP framework, please consult [tzhttpd](https://github.com/taozhijiang/tzhttpd/) and [tzmonitor](https://github.com/taozhijiang/tzmonitor/) preject.   


### 前言   
本项目来源于在公司做打款系统的时候，缺乏结算通道模拟器用于系统和路由的测试，于是就萌生开发一个最常用的HTTP/Json的通道模拟服务端。目前公司项目基本都是用传统CGI或者新型Thrift RPC接口形式来开发的，但是这两者在业务开发、客户端集成、部署、维护等方面都有一些缺陷，而一个可以处理业务逻辑，采用通用的HTTP和Json协议相对就会很方便。   
最初的这个原型比较的简单，但是通过项目的不断考验，不少问题得到修复，同时功能也得到丰富和增强。在比较稳定、可靠的HTTP服务框架下，用户可以快速定制自己的业务处理逻辑，从而快速开发满足自己特定需求的HTTP服务端原型，当然也可作为服务端开发的例子学习。   
该主干分支的代码都是稳定版本，在推送之前都进行了测试验证，当然人无完人，百密还是可能一疏，所以有任何问题或者建议，以希望不吝赐教。   

### 功能组件   
- 基于boost.asio框架库实现，理论上满足高并发、高性能的需求；   
- 支持HTTP协议的GET、POST请求的接收、参数解析，附带服务端常用的会话连接跟踪、限流等机制；   
- MySQL、RabbitMQ、Redis连接池，连接池可以动态伸缩和回收，方便、资源安全的操作接口；   
- 一个方便使用的定时器服务接口；   
- 方便可升缩的工作线程池设计；   

***注意***：除了上面的GET、POST，其他HTTP操作、特性、SSL都不支持，SSL可以使用Nginx前置卸载，其他少用的特性暂时也没考虑添加，实用性才是王道。   

### 性能   
在自己的渣本MF840上使用配置 2Core|1024M 的CentOS6虚拟机，服务设置采用3个io_service线程组，siege客户端设置1000并发强度下，基本能跑到1200trans/sec的吞吐量，更强的配置肯定能跑到更高的吞吐量，对于一般的小型服务应该是绰绰有余了。   
![siege](siege.png?raw=true "siege")

测试的时候注意修改系统的三个参数：   
```bash
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_timestamps=1
net.ipv4.tcp_tw_recycle=1
```

Enjoy it!   
