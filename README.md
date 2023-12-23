# Android Enhancer (formerly AOSP Enhancer) 🚀
A revolutionary android optimizer. 

## Variants 🧩
- [App](https://github.com/iamlooper/Android-Enhancer/tree/app)
- [Module](https://github.com/iamlooper/Android-Enhancer/tree/module)

## Download 📲
You can download Android Enhancer from the following:
- [Pling](https://www.pling.com/p/1875251)
- [Buy Me a Coffee](https://buymeacoffee.com/iamlooper/posts) (Early access)

## Description 📝
Android Enhancer is a specialized tool designed to optimize the performance of Android devices by modifying specific core parameters. Unlike other optimizers, Android Enhancer employs a universal approach, enabling it to function effectively across a wide range of Android devices. Consequently, it can enhance the performance of various devices, encompassing smartphones, tablets, and other Android-powered devices.

## Working ⚙️
To understand the functioning of the Android Enhancer, please examine the source code located [here](https://github.com/iamlooper/Android-Enhancer/blob/main/src/android_enhancer.cpp). The code currently lacks extensive documentation, but rest assured, I am actively working on improving it.

## Benchmarks ⚡
The given benchmarks were conducted on a Poco M3 running the GreenForce kernel on Pixel Experience Android 13.

### Scheduler latency via `hackbench` (Lower values indicate better performance)
- Without Android Enhancer: 0.919 seconds
- With Android Enhancer: 0.460 seconds

### Scheduler latency via `schbench` (Lower values indicate better performance)
- Without Android Enhancer:
`50.0th: 629
75.0th: 14480
90.0th: 32416
95.0th: 39232
*99.0th: 53696
99.5th: 60992
99.9th: 85888
min=0, max=125470`

- With Android Enhancer:
`50.0th: 351
75.0th: 13392
90.0th: 30432
95.0th: 37568
*99.0th: 53184
99.5th: 60096
99.9th: 82304
min=0, max=102169`

### Scheduler throughput via `perf bench sched messaging` (Lower values indicate better performance)
- Without Android Enhancer: 0.513 seconds
- With Android Enhancer: 0.466 seconds

### Scheduler throughput via `perf bench sched pipe` (Lower values indicate better performance)
- Without Android Enhancer: 31.806 seconds
- With Android Enhancer: 27.495 seconds

## Credits 👥
Due to the combined efforts and expertise of the following people, this project has achieved its success:
- [Chirag](https://t.me/selfmuser)
- [Leaf](https://t.me/leafinferno)
- [Jis G Jacob](https://t.me/StudioKeys)

Message me if I missed anyone. 😉

## Support Me 💙
If you liked any one of my projects then consider supporting me via following:
- [Buy Me a Coffee](https://buymeacoffee.com/iamlooper/membership)
- [Telegram Channel](https://loopprojects.t.me)