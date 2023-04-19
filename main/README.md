# How to enable SslSocket
Define PRIMESOCKET_USE_SSL in your headers beforce including PrimeSocket.h

# How to use class methods as callbacks
Define your class method as static and pass your class pointer ("this") to Socket functions as the parameter "dataPointers".
Your callback "dataPointers" variable now contains the pointer to your class when its called.
You can also pass any other object pointers including structures which makes this a powerful callback system.
