/**
 * network.h
 * 网络模块头文件
 *
 * 使用libcurl实现HTTP请求
 * 支持自动解压gzip/deflate响应
 */

#ifndef NETWORK_H
#define NETWORK_H

/**
 * 初始化网络模块
 * 必须在程序启动时调用一次
 */
void network_init(void);

/**
 * 清理网络模块
 * 在程序退出前调用
 */
void network_cleanup(void);

/**
 * 根据CID下载弹幕XML
 * @param cid 视频的内容ID (Content ID)
 * @return XML字符串内容（调用者需free释放），失败返回NULL
 */
char *fetch_danmaku(long long cid);

#endif // NETWORK_H
