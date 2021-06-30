# Short Description

The ring storage method for caching reads is slow. I've developed a hash
function that allows for a similar storage and lookup but much faster. I need to test this new method to show that it find a comparable number of hits to the near perfect method. I would also like to find out how much space is needed with respect to the size of the key space.

## mechanism

In the ring method in order to find a cached key each key is itterated over, and all of the old virtual addresses for that key are checked. The cache returns a hit if the value is found. 

In the new method I allocate a bit buffer, and take the address, shift it so that only the bits I want are present (>>32) and then modulo the size of the buffer. Whenever a write occurs it's possible that an old value is overwritten. To check if the value is correct I just store the virtual address in the hash. I check if the value being hashed is what is stored. It's a hit if they match. I have a second array that stores the key for that vaddr which I lookup on hits.

This algorithm has the advantage that it is very quick, with the downside that
there is a high probability of collision. The upshot here is that the older a
value gets the more likely it is to be overwritten. I don't really care if I
miss on a few collisions as long as the common case is fast enough.

## Experiment

In this first experiment I'm just testing the basic mechansim to make sure that
it scales up and that there is a performance gain to be had. The control in this
first experiment is just write steering. I allow all of the reads to pass
through. In the control I run the hash function on reads as they arrive. The
hashspace for this experiment is 1 >> 22. The function is a mod on the vaddr.

 - Hashspace: `1 >> 22`
 - H(X): `uint32_t index = (ntohl(vaddr >> 32) % HASHSPACE)`

![exp0](experiment_0.png "Read Cache with hash function (modulo)")

## Results

The results show that there is a gain to be had once there is enough contention
on the reads. I can only assume that this value will continue to grow as the
contention grows as well. The hash function does not seem to have much overhead
at the lower values.


