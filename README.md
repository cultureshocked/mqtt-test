# mqtt-test

Just rewriting the Eclipse Paho C MQTT examples. Mostly to get the feel for the library, but also because I think some parts look kinda weird.

## What's changed?

First off, I know that `goto` isn't inherently bad. There are valid uses, and the Paho C examples do, in fact, use it correctly.

That being said, I personally don't see many `goto`s in the C source I read, and as such, my rewrites will aim to be a bit more explicit with the branches, even if they result in the 'same outcome'. 

Second, I'm removing global variables by using a bit of abstraction whenever I can. There's the `src/status.c` file which has the same globals, but offers at least a bit of duty-separation with some helper functions.

Anything else that would get diff'd is probably just a naming convention change, and doesn't really change anything functionally. Maybe some extra debug statements here and there, too.

## License

EPL2.
