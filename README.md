# xmrig-kernel / Prefetch Mod
 My own kernel driver for the xmrig miner.
 
 This driver is designed to modify prefetching-related model-specific registers (MSRs), as memory prefetching causes performance loss. This is because of the RandomX algorithm used by Monero and its inherently randomized nature (hence its name).
 Disabling the prefetchers temporarily does not cause any slow downs elsewhere within RandomX, as all places where prefetches are beneficial have already been integrated within xmrig.

 This is also intended to be a completely secure alternative to the WinRing0 code-base which xmrig currently uses. This driver enforces a MSR whitelist to only allow certain registers to be read from or written to. It also integrates functionality to read the prefetching values of each CPU core and restore them on demand.
 
 In the future, I may also implement functionality to change the prefetching settings across all cores. Though, with how the code is structured, it may be rather tedious to do.
