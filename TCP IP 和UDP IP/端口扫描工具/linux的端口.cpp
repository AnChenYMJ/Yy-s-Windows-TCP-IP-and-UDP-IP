/*
Author: Adrian LaCour
Date: 12/3/2018
Course: CSCE 4550.001
Description: Basic port scanner to analyze for open ports.
Usage: ./portScan [option1, ..., optionN]
        executing program with no additional arguments runs the default, which is
Available options:
    --help (Displays invocation options)
    --port <Ports to scan>
    --ip <IP address to scan>
    --file <filename containing IP addresses to scan>
    --transport <TCP or UDP>
*/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <getopt.h>
#include <sys/select.h>
#include <vector>
#include <string>

using namespace std;

void defaultScan()
{
    int sockfd, sendsock, portNo, rval;
    struct timeval timeout;//For use in select() fucntion for UDP call
    fd_set s;//For use in select() fucntion for UDP call
    struct servent* appl_name; //Struct for use in getting service name
    struct sockaddr_storage their_addr;//For use in the rcvfrom() fucntion in UDP
    socklen_t addr_len;//For use in the rcvfrom() fucntion in UDP
    char proto[4] = "TCP";//Char string to set service type for getservbyport() fucntion
    struct hostent* hostaddr;   //To be used for IPaddress
    struct sockaddr_in servaddr, si_other;   //socket structure
    char* name;//Used to hold name of the port number's service
    int result;//To store the return value of the select() function in UDP calls
    char buf[256];
    int open = -1;//Used to detect if a port is open for UDP

    //Default (If there are no arguemtns given )
    cout << "Default port scan of 129.120.151.96    Ports: 1 - 1024\n";
    cout << "==========================TCP==========================\n";
    cout << "129.120.151.96\nTCP PORT\t  State\t\tService\n";
    for (int portNo = 1; portNo <= 1024; portNo++)//Parse through ports 1 - 1024
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);//Connects to the socket using TCP (SOCK_STREAM = TCP; SOCK_DGRAM = UDP)
        if (sockfd < 0)
            error(EXIT_FAILURE, 0, "ERROR opening socket");

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(portNo); //set the port number

        hostaddr = gethostbyname("129.120.151.96"); //IP address of CSE03 machine

        memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

        //Connect to default IP and rotating Port number
        appl_name = getservbyport(htons(portNo), NULL);//Get the service based on port number
        if (appl_name == NULL)
        {
            char tempChar[8] = "Unknown";
            name = tempChar;
        }
        else
        {
            name = appl_name->s_name;
        }

        rval = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (rval == -1)//If the socket descriptor is closed, don't print it
        {
            //printf("%d \t\t Closed \t %s\n", portNo, name);
        }
        else//If the socket descriptor is open
        {
            printf("%d \t\t Open \t\t %s\n", portNo, name);
        }

        close(sockfd);//Close the socket
    }
    //Do same thing, but with UDP, still the default setting
    cout << "==========================UDP==========================\nScanning may take a moment, as it waits for a response\n";
    cout << "129.120.151.96\nUDP PORT\t  State\t\tService\n";
    for (int portNo = 1; portNo <= 1024; portNo++)//Parse through ports 1 - 1024
    {
        bzero(buf, 256);

        //sendsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);//Connects to the socket using TCP (SOCK_STREAM = TCP; SOCK_DGRAM = UDP)
        if (sockfd < 0)
            error(EXIT_FAILURE, 0, "ERROR opening socket");

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(portNo); //set the port number

        hostaddr = gethostbyname("129.120.151.96"); //IP address of CSE03 machine

        memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

        rval = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));


        send(sockfd, "hello", 6, 0);

        timeout.tv_sec = 0;
        timeout.tv_usec = 450000;

        result = select(sockfd + 1, &s, NULL, NULL, &timeout);
        switch (result) {
        case 1:
            open = 0;
            break;

        case 0: // timeout
            open = 1;
            break;
        }

        appl_name = getservbyport(htons(portNo), NULL);//Get the service based on port number
        if (appl_name == NULL)
        {
            char tempChar[8] = "Unknown";
            name = tempChar;
        }
        else
        {
            name = appl_name->s_name;
        }

        if (open == 0)//If the socket descriptor is closed, don't print it
        {
            printf("%d \t\t Open \t\t %s\n", portNo, name);
        }
        else//If the socket descriptor is open
        {
            printf("%d \t\t Closed \t %s\n", portNo, name);
        }

        close(sockfd);
    }
}


int main(int argc, char* argv[])
{
    if (argc == 1)//If there are no extra flags/arguments, run the default code
    {
        defaultScan();
    }
    else
    {
        //Character array to hold the given arguments, to pass to the appropriate function, settling all of the flags
        char portArg[1024], ipArg[1024], fileArg[1024], transportArg[1024];
        memset(portArg, 0, 1024);//Zeros out the char array to 0
        memset(ipArg, 0, 1024);//Zeros out the char array to 0
        memset(fileArg, 0, 1024);//Zeros out the char array to 0
        memset(transportArg, 0, 1024);//Zeros out the char array to 0

        //Struct to handle the getopt_long function, for settling arguments
        static struct option long_options[] =
        {
            {"help", no_argument, NULL, 'h'},
            {"port", required_argument, NULL, 'p'},
            {"ip", required_argument, NULL, 'i'},
            {"file", required_argument, NULL, 'f'},
            {"transport", required_argument, NULL, 't'},
            {NULL, 0, NULL, 0}
        };
        //Loop through all of the given arguments, and determine what to do based on them
        char tempChar;//Used to detect if arguments are done, as getopt() returns -1
        while ((tempChar = getopt_long(argc, argv, "p:i:f:t:h", long_options, NULL)) != -1)
        {
            switch (tempChar)
            {
            case 'h': //If the help option is invoked, display the message and exit the program
                cout << "Available options:\n--help (Displays invocation options)\n--port <Ports to scan>\n--ip <IP address to scan>\n--file <filename containing IP addresses to scan>\n--transport <TCP or UDP>\n";
                return 0;
                break;
            case 'p': //If the port option is called, scan given ports, seperated by commas or a range, seperated by a -
                strcpy(portArg, optarg);//Concatatenates the arguemt flag tot eh c string
                break;
            case 'i': //If the ip option is called, scan given ports, seperated by commas or a range, seperated by a -
                strcpy(ipArg, optarg);//Concatatenates the arguemt flag tot eh c string
                break;
            case 'f': //If the file option is called, scan a given file for a list of ip addresses, each seperated by a line
                strcpy(fileArg, optarg);//Concatatenates the arguemt flag tot eh c string
                break;
            case 't': //If the transport option is called, only scan using the given port type, TCP or UDP
                strcpy(transportArg, optarg);//Concatatenates the arguemt flag tot eh c string
                break;
            }
        }


        //Start doing the non-defualt port scan, using the given arguments
        int sockfd, sendsock, portNo, rval;
        struct timeval timeout;//For use in select() fucntion for UDP call
        fd_set s;//For use in select() fucntion for UDP call
        struct servent* appl_name; //Struct for use in getting service name
        struct sockaddr_storage their_addr;//For use in the rcvfrom() fucntion in UDP
        socklen_t addr_len;//For use in the rcvfrom() fucntion in UDP
        char proto[4] = "TCP";//Char string to set service type for getservbyport() fucntion
        struct hostent* hostaddr;   //To be used for IPaddress
        struct sockaddr_in servaddr, si_other;   //socket structure
        char* name;//Used to hold name of the port number's service
        int result;//To store the return value of the select() function in UDP calls
        char buf[256];
        int open = -1;//Used to detect if a port is open for UDP
        vector<string> ipArgs;//Vector to hold all of the ip addresses
        vector<int> portArgs;//Vector to hold all of the port arguments
        bool portOption;//Variable to determine how to loop through the ports, as they are handled differently depending on input
                        //0 is only 1 port, or a dash. 1 is comma seperated ports
        char* hostIP;//Used to assign the argument for gethostbyname() fucntion

        //Handle if certian arguments were not given, by giving them a default value
        if (portArg[0] == '\0')//If there is no argument given
        {
            strcpy(portArg, "1-1024");
            for (int i = 1; i <= 1024; i++)
            {
                portArgs.push_back(i);
            }
            cout << "poop size = " << portArgs.size();
            portOption = 1;
        }
        else//If ports were given, assign them to a vector
        {
            char* pch;//For use in strtok
            pch = strtok(portArg, ",");
            if (pch == NULL)//If a ',' is not found
            {
                pch = strtok(portArg, "-");
                if (pch == NULL)//If there is no "," or "-", then there is only a single port
                {
                    //convert the cstring to an int. Store the int into the portArgs vector
                    portArgs.push_back(stoi(portArg));
                    portOption = 1;//Used to determine which for loop to use to loop through the ports.
                }
                else//If there is a "-", but no ','
                {
                    portArgs.push_back(stoi(pch));
                    pch = strtok(NULL, "\0");
                    portArgs.push_back(stoi(pch));
                    portOption = 0;//Used to determine which for loop to use to loop through the ports.
                }
            }
            else//If a ',' is found
            {
                while (pch != NULL)//Keep getting the port numbers, assigning them to the vector
                {
                    portArgs.push_back(stoi(pch));
                    pch = strtok(NULL, ",");
                }
                portOption = 1;//Used to determine which for loop to use to loop through the ports.
            }
        }

        if (fileArg[0] == '\0')//If there is no argument given
        {
            strcpy(fileArg, "None");
        }
        if (transportArg[0] == '\0')//If there is no argument given
        {
            strcpy(transportArg, "TCP and UDP");
        }


        //Handle the file in the ipArg
        if (strcmp(fileArg, "None") != 0)
        {
            FILE* fp;
            char line[256];
            char tempString[256];
            size_t len = 0;
            ssize_t read;

            fp = fopen(fileArg, "r");//Open the file
            if (fp == NULL)
                exit(EXIT_FAILURE);

            strcpy(ipArg, "");
            while (fgets(line, 256, fp) != NULL)
            {
                strcat(ipArg, line);
                strcat(ipArg, ",");

                strncpy(tempString, line, strlen(line) - 1);
                ipArgs.push_back(tempString);
            }

            fclose(fp);//Close the file
        }
        else if (strcmp(ipArg, "\0") == 0)//If there is no argument given for the ip or file, giving a default ip address
        {
            strcpy(ipArg, "129.120.151.96");
            ipArgs.push_back(ipArg);
        }
        else//If the ip argument is given
        {
            char* pch;//For use in strtok
            pch = strtok(ipArg, ",");
            if (pch == NULL)//If a ',' is not found
            {
                pch = strtok(ipArg, "-");
                if (pch == NULL)//If there is no "," or "-", then there is only a single port
                {
                    //convert the cstring to an int. Store the int into the portArgs vector
                    ipArgs.push_back(ipArg);
                }
                else//If there is a "-", but no ','
                {
                    ipArgs.push_back(pch);
                    pch = strtok(NULL, "\0");
                    ipArgs.push_back(pch);
                }
            }
            else//If a ',' is found
            {
                while (pch != NULL)//Keep getting the port numbers, assigning them to the vector
                {
                    ipArgs.push_back(pch);
                    pch = strtok(NULL, ",");
                }
            }
        }

        //Run the scan on TCP ports, if it is default or chosen to be TCP
        if ((strcmp(transportArg, "TCP") == 0) || (strcmp(transportArg, "TCP and UDP") == 0))
        {
            for (int i = 0; i < ipArgs.size(); i++)//Loop through all of the ip addresses
            {
                if (portOption == 0)//Ifthere is a dash for the ports
                {
                    //cout << "Port scan of " << ipArgs[i] << "    Ports: " << portArgs[k] << endl;
                    cout << "==========================" << "TCP" << "==========================\n";
                    cout << ipArgs[i] << "\n" << "TCP" << " PORT\t  State\t\tService\n";

                    for (int k = portArgs[0]; k <= portArgs[1]; k++)//Loop through all of the ports
                    {
                        sockfd = socket(AF_INET, SOCK_STREAM, 0);//Connects to the socket using TCP (SOCK_STREAM = TCP; SOCK_DGRAM = UDP)
                        if (sockfd < 0)
                            error(EXIT_FAILURE, 0, "ERROR opening socket");

                        memset(&servaddr, 0, sizeof(servaddr));

                        servaddr.sin_family = AF_INET;
                        servaddr.sin_port = htons(k); //set the port number

                        hostaddr = gethostbyname(ipArgs[i].c_str()); //IP address of CSE03 machine

                        memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

                        //Connect to default IP and rotating Port number
                        appl_name = getservbyport(htons(k), NULL);//Get the service based on port number
                        if (appl_name == NULL)
                        {
                            char tempChar[8] = "Unknown";
                            name = tempChar;
                        }
                        else
                        {
                            name = appl_name->s_name;
                        }

                        rval = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                        if (rval == -1)//If the socket descriptor is closed, don't print it
                        {
                            printf("%d \t\t Closed \t %s\n", k, name);
                        }
                        else//If the socket descriptor is open
                        {
                            printf("%d \t\t Open \t\t %s\n", k, name);
                        }

                        close(sockfd);//Close the socket
                    }
                }
                else if (portOption == 1)//If there is only a single port, or it is comma seperated
                {
                    //cout << "Port scan of " << ipArgs[i] << "    Ports: " << portArgs[k] << endl;
                    cout << "==========================" << "TCP" << "==========================\n";
                    cout << ipArgs[i] << "\n" << "TCP" << " PORT\t  State\t\tService\n";

                    for (int k = 0; k < portArgs.size(); k++)//Loop through all of the ports
                    {
                        sockfd = socket(AF_INET, SOCK_STREAM, 0);//Connects to the socket using TCP (SOCK_STREAM = TCP; SOCK_DGRAM = UDP)
                        if (sockfd < 0)
                            error(EXIT_FAILURE, 0, "ERROR opening socket");

                        memset(&servaddr, 0, sizeof(servaddr));

                        servaddr.sin_family = AF_INET;
                        servaddr.sin_port = htons(portArgs[k]); //set the port number

                        //strcpy(hostIP, ipArgs[i].c_str());
                        hostaddr = gethostbyname(ipArgs[i].c_str()); //IP address of CSE03 machine

                        memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

                        //Connect to default IP and rotating Port number
                        appl_name = getservbyport(htons(portArgs[k]), NULL);//Get the service based on port number
                        if (appl_name == NULL)
                        {
                            char tempChar[8] = "Unknown";
                            name = tempChar;
                        }
                        else
                        {
                            name = appl_name->s_name;
                        }

                        rval = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                        if (rval == -1)//If the socket descriptor is closed, don't print it
                        {
                            printf("%d \t\t Closed \t %s\n", portArgs[k], name);
                        }
                        else//If the socket descriptor is open
                        {
                            printf("%d \t\t Open \t\t %s\n", portArgs[k], name);
                        }

                        close(sockfd);//Close the socket
                    }
                }//End of comma sperated option
            }//End of IP loop
        }//End of TCP loop
        //Run the scan on UDP ports, if it is the default or chosen to be UDP
        if ((strcmp(transportArg, "UDP") == 0) || (strcmp(transportArg, "TCP and UDP") == 0))
        {
            for (int i = 0; i < ipArgs.size(); i++)//Loop through all of the ip addresses
            {
                if (portOption == 0)//Ifthere is a dash for the ports
                {
                    //cout << "Port scan of " << ipArgs[i] << "    Ports: " << portArgs[k] << endl;
                    cout << "==========================" << "UDP" << "==========================\nScanning may take a moment, as it waits for a response\n";
                    cout << ipArgs[i] << "\n" << "UDP" << " PORT\t  State\t\tService\n";

                    for (int k = portArgs[0]; k <= portArgs[1]; k++)//Loop through all of the ports
                    {
                        bzero(buf, 256);

                        //sendsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                        sockfd = socket(AF_INET, SOCK_DGRAM, 0);//Connects to the socket using TCP (SOCK_STREAM = TCP; SOCK_DGRAM = UDP)
                        if (sockfd < 0)
                            error(EXIT_FAILURE, 0, "ERROR opening socket");

                        memset(&servaddr, 0, sizeof(servaddr));

                        servaddr.sin_family = AF_INET;
                        servaddr.sin_port = htons(k); //set the port number

                        hostaddr = gethostbyname(ipArgs[i].c_str()); //IP address of CSE03 machine

                        memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

                        rval = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

                        timeout.tv_sec = 0;
                        timeout.tv_usec = 450000;
                        send(sockfd, NULL, 0, 0);

                        result = select(sockfd + 1, &s, NULL, NULL, &timeout);
                        switch (result) {
                        case 1:
                            open = 0;
                            break;

                        case 0: // timeout
                            open = 1;
                            break;
                        }

                        appl_name = getservbyport(htons(k), NULL);//Get the service based on port number
                        if (appl_name == NULL)
                        {
                            char tempChar[8] = "Unknown";
                            name = tempChar;
                        }
                        else
                        {
                            name = appl_name->s_name;
                        }

                        if (open == 0)//If the socket descriptor is closed, don't print it
                        {
                            printf("%d \t\t Open \t\t %s\n", portArgs[k], name);
                        }
                        else//If the socket descriptor is open
                        {
                            printf("%d \t\t Closed \t %s\n", portArgs[k], name);
                        }

                        close(sockfd);
                    }
                }
                else if (portOption == 1)//If there is only a single port, or it is comma seperated
                {
                    //cout << "Port scan of " << ipArgs[i] << "    Ports: " << portArgs[k] << endl;
                    cout << "==========================" << "UDP" << "==========================\nScanning may take a moment, as it waits for a response\n";
                    cout << ipArgs[i] << "\n" << "UDP" << " PORT\t  State\t\tService\n";

                    for (int k = 0; k < portArgs.size(); k++)//Loop through all of the ports
                    {
                        bzero(buf, 256);

                        //sendsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                        sockfd = socket(AF_INET, SOCK_DGRAM, 0);//Connects to the socket using TCP (SOCK_STREAM = TCP; SOCK_DGRAM = UDP)
                        if (sockfd < 0)
                            error(EXIT_FAILURE, 0, "ERROR opening socket");

                        memset(&servaddr, 0, sizeof(servaddr));

                        servaddr.sin_family = AF_INET;
                        servaddr.sin_port = htons(portArgs[k]); //set the port number

                        hostaddr = gethostbyname(ipArgs[i].c_str()); //IP address of CSE03 machine

                        memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

                        rval = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

                        timeout.tv_sec = 0;
                        timeout.tv_usec = 450000;
                        send(sockfd, NULL, 0, 0);

                        result = select(sockfd + 1, &s, NULL, NULL, &timeout);
                        switch (result) {
                        case 1:
                            open = 0;
                            break;

                        case 0: // timeout
                            open = 1;
                            break;
                        }

                        appl_name = getservbyport(htons(portArgs[k]), NULL);//Get the service based on port number
                        if (appl_name == NULL)
                        {
                            char tempChar[8] = "Unknown";
                            name = tempChar;
                        }
                        else
                        {
                            name = appl_name->s_name;
                        }

                        if (open == 0)//If the socket descriptor is closed, don't print it
                        {
                            printf("%d \t\t Open \t\t %s\n", portArgs[k], name);
                        }
                        else//If the socket descriptor is open
                        {
                            printf("%d \t\t Closed \t %s\n", portArgs[k], name);
                        }

                        close(sockfd);
                    }
                }//End of comma sperated option
            }//End of IP loop
        }//End op UDP loop
    }//End of else, for if it isnt just a default run, with no argument

    return 0;
}