/**
 * @file client.cpp
 * @author lzx0626 (2065666169@qq.com)
 * @brief 客户端主程序（仅用作服务端功能测试，真正的客户端使用 Qt 编写）
 * @version 1.0
 * @date 2024-03-01
 *
 * Copyright (c) 2023 电子科技大学 刘治学
 *
 */

#include <iostream>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "tools.h"

/**
 * @brief 客户端程序，仅用作服务端功能测试，连接服务端，并且作信息交互
 */
int main(int argc, char *const argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage: ./client  <serverIp>  <serverPort>\n";
        return -1;
    }

    // 1. 创建 socket 套接字
    int connectFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == connectFd)
    {
        perror("socket");
        return -1;
    }

    // 为了更快的读取图片文件，可以考虑修改一下内核中接收缓冲区的大小，例如改为 0.5 MB
    // 但是鉴于我们的素材图片都非常小，十几KB ，接收缓冲区是完全够用的，我这边那我自己的数组（注意数组不是接收缓冲区）循环读取即可，不用担心接收缓冲区被撑爆
    // int recvBuf = 1024 * 512;
    // int res = setsockopt(connectFd, SOL_SOCKET, SO_RCVBUF, &recvBuf, sizeof(recvBuf));
    // if (-1 == res)
    // {
    //     perror("sersockopt");
    //     return -1;
    // }

    // 2. 连接服务端
    struct sockaddr_in serverAddr;
    // 地址族
    serverAddr.sin_family = AF_INET;
    // IP
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr.s_addr);
    // 端口
    serverAddr.sin_port = htons(atoi(argv[2]));

    int res = connect(connectFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (-1 == res)
    {
        perror("connect");
        return -1;
    }

    std::cout << "connect to server successfully." << std::endl;

    // 3. 通信
    char buf[maxBufferSize + 1] = {0};

    while (1)
    {
        bzero(buf, sizeof(buf));
        fgets(buf, sizeof(buf) - 1, stdin);

        // 写
        // 我们自定义客户端发送的信息，一次性能全部发完，所以这里就不做循环发送了
        send(connectFd, buf, strlen(buf), 0);
        std::cout << "send: " << buf;

        // 读
        // 读取图片的个数
        int len = -1;

        bzero(buf, sizeof(buf));
        len = recv(connectFd, buf, sizeof(buf) - 1, 0);

        if (-1 == len)
        {
            perror("recv");
            return -1;
        }

        // 服务端关闭
        if (0 == len)
        {
            std::cout << "server has closed." << std::endl;
            close(connectFd);
            exit(-1);
        }

        else
        {
            // 这里可能是正常手动退出，服务端发送的回信
            if (0 == strcmp("exit success\n", buf))
            {
                close(connectFd);
                exit(-1);
            }

            int fileNum = atoi(buf);
            for (int i = 0; i < fileNum; ++i)
            {
                std::vector<std::string> encodedList;
                std::string outputFilePath;
                int num = 0; // 第一次发送的数据是图片的名称

                while (1)
                {
                    bzero(buf, sizeof(buf));
                    len = recv(connectFd, buf, sizeof(buf) - 1, 0);
                    if (-1 == len)
                    {
                        perror("recv");
                        return -1;
                    }

                    // 服务端关闭
                    if (0 == len)
                    {
                        std::cout << "server has closed." << std::endl;
                        close(connectFd);
                        exit(-1);
                    }
                    // 正常通信
                    else
                    {
                        // 传输结束
                        if (0 == strcmp("send over\n", buf))
                        {
                            std::cout << "\nrecv over pic number " << (1 + i) << "\n";
                            break;
                        }

                        // 第一次
                        if (0 == num++)
                        {
                            std::string fileNameSize = buf;
                            size_t pos = fileNameSize.find('\n');
                            std::string fileName = fileNameSize.substr(0, pos);
                            std::string fileSize = std::string{fileNameSize.begin() + pos + 1, fileNameSize.end()};

                            std::cout << fileName << '\n'
                                      << fileSize << '\n';

                            outputFilePath = std::string("../res/") + fileName;
                            // 加上 _copy 后缀
                            for (int i = 0; i < 4; ++i)
                                outputFilePath.pop_back();
                            outputFilePath += "_copy.png";

                            continue;
                        }

                        // 我们默认数据传输过程中不会出现问题
                        // 经过 tools 中的处理，这里的 buf 每一个最多都只能是 1024 字节，不会溢出
                        std::cout << buf;
                        encodedList.push_back(buf);
                    }
                }

                tools::decodeAndOutputToFile(outputFilePath.c_str(), encodedList);
            }
        }
    }

    // 4. 关闭套接字
    close(connectFd);

    return 0;
}
