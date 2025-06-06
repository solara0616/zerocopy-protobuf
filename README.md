Bosch's Iceoryx autonomous driving middleware is a communication middleware that achieves true zero-copy transmission. It employs C struct-style IDL but only supports fixed-length message types. While Google's Protocol Buffers (protobuf) supports variable-length messages, its software architecture makes true shared-memory zero-copy challenging to implement. This project modifies protobuf's source code implementation, integrating Iceoryx's approach to enable shared-memory zero-copy communication based on protobuf's variable-length message structure.

Compilation method:
tools/iceoryx_build_test.sh example

Execution:
Run separately:
./proto_zerocopy_subscriber
and
./proto_zerocopy_publisher

![time delay 1 (zero copy VS non zero copy)](https://github.com/solara0616/zerocopy-protobuf/blob/main/iceoryx_examples/protobuf/time_delay_comparison_1.png)
![time delay 2 (zero copy VS non zero copy)](https://github.com/solara0616/zerocopy-protobuf/blob/main/iceoryx_examples/protobuf/time_delay_comparison_2.png)
