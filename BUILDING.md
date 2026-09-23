<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>

<meta http-equiv="content-type" content="text/sgml;charset=utf-8">

<style type="text/css">

body {background: white;color: black}
h1 {color: #c33;background: none;font-weight: bold;text-align: center}
h2 {color: #00008b;background: none;font-weight: bold}
h3 {color: #006400;background: none;margin-left: 4%;margin-right: 4%;font-weight: bold}
h4 {margin-left: 8%;margin-right: 8%;font-weight: bold}
h5 {margin-left: 6%;margin-right: 6%;font-weight: bold}
div {margin-left: 10%;margin-right: 10%;}
ul, ol, dl, p {margin-left: 6%;margin-right: 6%}
pre {margin-left: 10%;white-space: pre}

</style><title>Building LinBPQ from source</title>

</head>
<body alink="#ff0000" bgcolor="#ffffff" link="#0000ff" text="#000000" vlink="#800080">
<a href="index.html">BPQ Home</a>&nbsp;&nbsp;&nbsp;<a href="BPQ32.html">BPQ32 Home</a>

<h1>Building LinBPQ from source.</h1>

<p>
I provide binaries for Linux on 32 and 64 bit ARM and Intel x86 processors but if you want to run on other Unix like operating systems you can build it from source. You will need to install some dependencies and download the source. The instructions below are for a Debian like system using apt as its package manager - you may need to alter the command for other systems. 
<br><br>
To download dependencies run:
<br>
<pre>sudo apt install build-essential git libminiupnpc-dev libconfig-dev libpcap-dev zlib1g-dev  libcap2-bin libjansson-dev libpaho-mqtt-dev libbacktrace-dev
</pre>
<p>
Some distributions may not include libpaho-mqtt-dev and/or libbacktrace-dev. The former is used for the MQTT reporting interface. If you don't need that you can build without MQTT support. If you do you can build libpaho-mqtt-dev from source:
<pre>
sudo apt install cmake
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
cmake -Bbuild -H. -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_STATIC=ON -DPAHO_WITH_SSL=OFF -DPAHO_HIGH_PERFORMANCE=ON -DCMAKE_INSTALL_PREFIX=../install
cd build
cmake --build . --target install

You need to copy libpaho-mqtt3a.a to your lib directory.
The commands below work on my ARM and Intel systems but you may need to change if your lib is in a different place.

sudo cp src/libpaho-mqtt3a.a /usr/lib/arm-linux-gnueabihf 
or
sudo cp src/libpaho-mqtt3a.a /usr/lib/i386-linux-gnu/
</pre><p>
If libbacktrace isn't available you can build it:
<pre>
git clone https://github.com/ianlancetaylor/libbacktrace.git
cd libbacktrace 
mkdir build 
cd build 
../configure 
make
sudo make install
</pre><p>

To download the LinBPQ source run:
<pre>
git clone https://github.com/g8bpq/linbpq.git
cd linbpq</pre>
<p>
To build with MQTT support enter:
<pre>
make
</pre>
<p>
To build without MQTT support enter:
<pre>
make nomqtt
</pre><p>
This should create linbpq in the current directory.
<br><br>
</P>


John Wiseman G8BPQ<br>
April 2025<br>
Updated September 2026<br>

</body>
</html>