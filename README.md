# shadow_talkie

*A decentralized chat program w/ p2p encryption*

##### *Router Screenshot:*
![image](https://user-images.githubusercontent.com/25305842/39403804-3931d9be-4b4a-11e8-9a3e-c738795ef839.png)
##### *Client Screenshot:*
![image](https://user-images.githubusercontent.com/25305842/39403806-40ec19d0-4b4a-11e8-830f-17cceb5bce14.png)

## Deploy

``` bash

git clone git@github.com:zhang13music/shadow_talkie.git
cd shadow_talkie

```
### for router:
``` bash

make router
./router

```
### for client:
``` bash

make talkie
./talkie

```
> NOTE: For successful connection, keep router and clients connected to the same local network and MAKE SURE to modify `ROUTER_HOST` macro under `includes/lib.h` to correctly match router's local IPv4 IP address before make.

- encryption: under construction 


