LV2 Test
========

This is a testing tool I hacked together to allow me to easily test lv2 plugins
without going through the hassle of manually connecting ports etc.

The idea is, you create input and output buffers, fill the inputs with data,
then run the plugin and check that the output buffers contain the data you
expect.

See example.c for an example of how to use it.

This is just hacked together for my own convenience, it's entirely likely it
won't do whatever you need it to, but feel free to improve it and submit a pull
request.
