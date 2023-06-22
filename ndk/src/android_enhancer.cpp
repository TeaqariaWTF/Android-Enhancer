#include <iostream> 
#include <unistd.h>
#include <cstring>
#include <vector>
#include <fstream>
#include <cerrno>
#include <dirent.h>

#include "util_functions.hpp"

// Set standard namespace.
using namespace std;

// Declare functions.
void kernel_tweak();
void io_tweak();
void priority_tweak();
void cmd_services_tweak();
void disable_unnecessary_services();
void props_tweak();
void dalvik_props_tweak();
void main_dex_tweak();
void secondary_dex_tweak();
void clean_junk();
void enable_mem_preload_tweak();
void disable_mem_preload_tweak();
void disable_miui_apps();
void enable_miui_apps();
void miui_props_tweak();
void disable_unnecessary_miui_services();
void apply_all_tweaks();
void apply_main_tweaks();
void apply_miui_tweaks();
void apply_dalvik_tweaks();

void kernel_tweak() {
  // Allow sched boosting on top-app tasks.  
  write("/proc/sys/kernel/sched_min_task_util_for_colocation", "0");

  // Turn off printk logging and scheduler stats.
  write("/proc/sys/kernel/printk", "0 0 0 0");
  write("/proc/sys/kernel/printk_devkmsg", "off");
  write("/proc/sys/kernel/sched_schedstats", "0");

  // Disable swap readahead for swap devices.
  write("/proc/sys/vm/page-cluster", "0");

  // Update /proc/stat every 360 sec to reduce jitter.
  write("/proc/sys/vm/stat_interval", "360");

  // Print tweak completion message.   
  xlog("date", "Applied universal kernel tweak.");  
}

void io_tweak() {
  // Initialize vector for paths and get paths from wildcard path using get_paths_from_wp() function.
  vector<string> paths;
  paths = get_paths_from_wp("/sys/block/*");
  // Iterate through the list of paths.
  for (const string& path : paths) {
    // Read the list of available schedulers from the scheduler file.
    ifstream file(path + "/queue/scheduler");
    string avail_scheds((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    // Set the scheduler to the first one in the list that is available.
    vector<string> scheds = {"cfq", "noop", "deadline", "none"};
    for (const string& sched : scheds) {
      if (avail_scheds.find(sched) != string::npos) {
        write(path + "/queue/scheduler", sched);
        break;
      }
    }
    
    // Disable I/O statistics, random I/O addition, and I/O polling.
    write(path + "/queue/iostats", "0");
    write(path + "/queue/add_random", "0");
    write(path + "/queue/io_poll", "0");
    // Set the idle time for groups and slices to 1 and 0, respectively.
    write(path + "/queue/iosched/group_idle", "1");
    write(path + "/queue/iosched/slice_idle", "0"); 
  }
  
  // Print tweak completion message.   
  xlog("date", "Tweaked I/O parameters.");  
}

void priority_tweak() {
  // Initialize variables.
  vector<string> tasks;
  vector<string> threads;

  // Core android process. (zygote)
  // Important display & graphics related process. (surfaceflinger)
  // Core services container processes. (system & system_server)
  // Composer thread i.e Hardware compose vsync & uevent. (composer)
  // Max renice with CPU affinity to all cores.
  tasks = {
    "zygote",
    "system",
    "system_server",      
    "surfaceflinger", 
    "composer"
  };
  // Iterate through the list of tasks. 
  for (const string& task : tasks) {
    renice_process(task, -20);
    change_process_cpu_affinity(task, 0, CPU_SETSIZE - 1);
  }
  
  // Kernel helper, service manager, allocator & storage daemon.
  // Renice only.
  tasks = {
    "khelper", 
    "servicemanager", 
    "allocator", 
    "storaged"
  };
  // Iterate through the list of tasks.    
  for (const string& task : tasks) {
    renice_process(task, -18);
  }  
  
  // Less priority with CPU affinity limited to CPU0.
  tasks = {
    "log", 
    "stats"
  };
  // Iterate through the list of tasks.
  for (const string& task : tasks) {
    renice_process(task, 12);
    change_process_cpu_affinity(task, 0, 0);  
  }  
  
  // SystemUI, Launcher and IME (Input method editor).
  // Change CPU affinity to all cores.
  tasks = {
    "com.android.systemui", 
    get_home_pkg_name(), 
    get_ime_pkg_name()
  };
  // Iterate through the list of tasks.
  for (const string& task : tasks) {
    change_process_cpu_affinity(task, 0, CPU_SETSIZE - 1);
  }
  
  // Threads of system server.
  // Max renice with CPU affinity to all cores.
  threads = {
    "android.fg", 
    "android.ui", 
    "android.io", 
    "android.display"
  };
  // Iterate through the list of threads.
  for (const string& thread : threads) {
    renice_thread("system_server", thread, -20);
    change_thread_cpu_affinity("system_server", thread, 0, CPU_SETSIZE - 1);
  } 

  // Less priority with CPU affinity limited to CPU0.
  threads = {
    "CpuTracker", 
    "watchdog", 
    "TaskSnapshot", 
    "StatsCompanion", 
    "batterystats", 
    "NetworkStats", 
    "GraphicsStats"
  };
  // Iterate through the list of threads.
  for (const string& thread : threads) {
    renice_thread("system_server", thread, 12);
    change_thread_cpu_affinity("system_server", thread, 0, 0);
  }

  // Print tweak completion message. 
  xlog("date", "Optimized priority of system processes.");  
}

// Tweak to disable debugging, statistics, unnecessary background apps, phantom process killer, lock profiling (analysis tool), make changes to `DeviceConfig` flags persistent, enable IORAP readahead feature, improve FSTrim time interval and tweak `ActivityManager`.
void cmd_services_tweak() {
  // List of service commands of `cmd` to execute.
  vector<string> svc_cmds = {
    "settings put system anr_debugging_mechanism 0",
    "looper_stats disable",
    "settings put global netstats_enabled 0",  
    "appops set com.android.backupconfirm RUN_IN_BACKGROUND ignore",
    "appops set com.google.android.setupwizard RUN_IN_BACKGROUND ignore",
    "appops set com.android.printservice.recommendation RUN_IN_BACKGROUND ignore",
    "appops set com.android.onetimeinitializer RUN_IN_BACKGROUND ignore",
    "appops set com.qualcomm.qti.perfdump RUN_IN_BACKGROUND ignore",
    "power set-fixed-performance-mode-enabled true",
    "activity idle-maintenance",
    "thermalservice override-status 1",
    "settings put global settings_enable_monitor_phantom_procs false",
    "device_config put runtime_native_boot disable_lock_profiling true",
    "device_config set_sync_disabled_for_tests persistent",
    "device_config put runtime_native_boot iorap_readahead_enable true",
    "settings put global fstrim_mandatory_interval 3600",
    "device_config put activity_manager max_phantom_processes 2147483647",
    "device_config put activity_manager max_cached_processes 256",
    "device_config put activity_manager max_empty_time_millis 43200000"
  };
  
  // Iterate through the list of service commands.
  for (const string& svc_cmd : svc_cmds) {
    // Use exec_shell() function to execute shell command.
    exec_shell("cmd " + svc_cmd, false);  
  }  

  // Print tweak completion message.
  xlog("date", "Tweaked `cmd` services.");
}

// Stop unnecessary services.
void disable_unnecessary_services() {
  // List of processes to be killed.
  vector<string> processes = {
    "traced",   
    "statsd", 
    "tcpdump", 
    "cnss_diag",     
    "ipacm-diag", 
    "ramdump", 
    "subsystem_ramdump", 
    "charge_logger"
  };

  // Iterate through the list of processes.
  for (const string& process : processes) {
    // Use kill_process() function to kill process.
    kill_process(process);
  }

  // Print tweak completion message.   
  xlog("date", "Disabled unnecessary services.");  
}

void props_tweak() {
  // List of properties to be modified.
  vector<string> props = {
    "persist.sys.usap_pool_enabled true", 
    "persist.device_config.runtime_native.usap_pool_enabled true", 
    "ro.iorapd.enable true", 
    "vidc.debug.level 0",
    "persist.radio.ramdump 0",
    "ro.statsd.enable false",
    "persist.debug.sf.statistics 0",
    "debug.mdpcomp.logs 0",
    "ro.lmk.debug false",
    "ro.lmk.log_stats false",
    "debug.sf.enable_egl_image_tracker 0",
    "persist.ims.disableDebugLogs 1",
    "persist.ims.disableADBLogs 1",
    "persist.ims.disableQXDMLogs 1",
    "persist.ims.disableIMSLogs 1" 
  };
  
  // Iterate through the list of properties.
  for (const string& prop : props) {
    // Use exec_shell() function to execute shell command.
    exec_shell("resetprop " + prop, false);  
  }

  // Print tweak completion message. 
  xlog("date", "Tweaked system properties.");    
}

void dalvik_props_tweak() {
  // List of properties to be modified.
  vector<string> props = {
    "dalvik.vm.minidebuginfo false", 
    "dalvik.vm.dex2oat-minidebuginfo false",
    "dalvik.vm.check-dex-sum false", 
    "dalvik.vm.checkjni false",
    "dalvik.vm.verify-bytecode false", 
    "dalvik.vm.lockprof.threshold 250",
    "dalvik.gc.type precise",
    "persist.bg.dexopt.enable false", 
    "pm.dexopt.forced-dexopt everything",
    "pm.dexopt.install speed", 
    "pm.dexopt.bg-dexopt verify",
    "pm.dexopt.boot verify",
    "pm.dexopt.first-boot quicken", 
    "pm.dexopt.ab-ota speed-profile",
    "dalvik.vm.systemservercompilerfilter speed"    
  };
  
  // Iterate through the list of properties.
  for (const string& prop : props) {
    // Use exec_shell() function to execute shell command.
    exec_shell("resetprop " + prop, false);  
  }

  // Print tweak completion message.   
  xlog("date", "Applied dalvik optimization props.");  
}

// Tweak to improve main DEX files.
void main_dex_tweak() {
  // Build command to execute.
  string cmd = "cmd package compile -m speed-profile -a";

  // Use exec_shell() function to execute shell command.
  exec_shell(cmd, false);   

  // Print tweak completion message.   
  xlog("date", "Applied main DEX tweak.");  
}

// Tweak to improve secondary DEX files.
void secondary_dex_tweak() {
  // List of shell commands to execute.
  vector<string> cmds = {
    "pm compile -m speed-profile --secondary-dex -a", 
    "pm reconcile-secondary-dex-files -a", 
    "pm compile --compile-layouts -a"
  };

  // Iterate through the list of commands.
  for (const string& cmd : cmds) {
    // Use exec_shell() function to execute shell command.
    exec_shell(cmd, false);  
  }

  // Print tweak completion message.   
  xlog("date", "Applied secondary DEX tweak.");  
}

void clean_junk() {
  // List of items to be cleaned.   
  vector<string> items = {
    "/data/*.log", 
    "/data/vendor/wlan_logs", 
    "/data/*.txt", 
    "/cache/*.apk",
    "/data/anr/*",
    "/data/backup/pending/*.tmp",
    "/data/cache/*",
    "/data/data/*.log",
    "/data/data/*.txt",
    "/data/log/*.log",
    "/data/log/*.txt",
    "/data/local/*.apk",
    "/data/local/*.log",
    "/data/local/*.txt",
    "/data/mlog/*",
    "/data/system/*.log",
    "/data/system/*.txt",
    "/data/system/dropbox/*",
    "/data/system/usagestats/*",
    "/data/tombstones/*",
    "/sdcard/LOST.DIR",
    "/sdcard/found000",
    "/sdcard/LazyList",
    "/sdcard/albumthumbs",  
    "/sdcard/kunlun",
    "/sdcard/.CacheOfEUI",
    "/sdcard/.bstats",
    "/sdcard/.taobao",
    "/sdcard/Backucup",
    "/sdcard/MIUI/debug_log",
    "/sdcard/UnityAdsVideoCache",
    "/sdcard/*.log",
    "/sdcard/*.CHK",
    "/sdcard/duilite",
    "/sdcard/DkMiBrowserDemo",
    "/sdcard/.xlDownload"
  };
  
  // Iterate through the list of items.
  for (const string& item : items) {
    // Use remove_path() function to remove item.
    remove_path(item); 
  }

  // Print tweak completion message.   
  xlog("date", "Cleaned junk from system.");      
}

// Tweak to preload important system objects into memory.
void enable_mem_preload_tweak() {
  vector<string> apex_objects = {
    "linker",
    "linker64",
    "libm.so",
    "libdl.so"
  };
  for (const string& apex_object : apex_objects) {
    if (is_path_exists("/apex/com.android.runtime/bin/" + apex_object)) {
      preload_item("obj", "/apex/com.android.runtime/bin/" + apex_object);
    } 
    if (is_path_exists("/apex/com.android.runtime/lib/bionic/" + apex_object)) {
      preload_item("obj", "/apex/com.android.runtime/lib/bionic/" + apex_object);
    } 
    if (is_path_exists("/apex/com.android.runtime/lib64/bionic/" + apex_object)) {
      preload_item("obj", "/apex/com.android.runtime/lib64/bionic/" + apex_object);
    }         
  }
  
  vector<string> bin_objects = {
    "dalvikvm",
    "app_process"
  };
  for (const string& bin_object : bin_objects) {
    if (is_path_exists("/bin/" + bin_object)) {
      preload_item("obj", "/bin/" + bin_object);
    }    
  }  

  vector<string> lib_objects = {     
    "libandroid.so",        
    "libandroid_runtime.so",
    "libandroid_servers.so",    
    "libEGL.so", 
    "libGLESv1_CM.so",      
    "libGLESv2.so", 
    "libGLESv3.so",       
    "libsurfaceflinger.so",    
    "libhwui.so",
    "libui.so",
    "libskia.so",     
    "libvulkan.so", 
    "libRScpp.so",
    "libinputflinger.so",  
    "libharfbuzz_ng.so",  
    "libprotobuf-cpp-full.so", 
    "libprotobuf-cpp-lite.so", 
    "libblas.so",
    "libutils.so"    
  };
  for (const string& lib_object : lib_objects) {
    if (is_path_exists("/system/lib/" + lib_object)) {
      preload_item("obj", "/system/lib/" + lib_object);
    } 
    if (is_path_exists("/system/lib64/" + lib_object)) {
      preload_item("obj", "/system/lib64/" + lib_object);    
    } 
  }

  vector<string> framework_objects = {
    "framework.jar",
    "services.jar",
    "protobuf.jar"
  };
  for (const string& framework_object : framework_objects) {
    if (is_path_exists("/system/framework/" + framework_object)) {
      preload_item("obj", "/system/framework/" + framework_object);
    }    
  }  

  vector<string> pkg_names = {
    "com.android.systemui",
    get_home_pkg_name(),
    get_ime_pkg_name()
  };   
  for (const string& pkg_name : pkg_names) {
    preload_item("app", pkg_name);
  }

  // Print tweak completion message.  
  xlog("date", "Preloaded important system items into RAM.");  
}

void disable_mem_preload_tweak() {
  // Kill `vmtouch` processes.
  kill_processes("vmtouch");

  // Print tweak completion message.   
  xlog("date", "Disabled memory preload tweak.");  
}

// Tweak to disable unnecessary MIUI apps.
void disable_miui_apps() {
  // List of apps to be disabled.
  vector<string> apps = {
    "com.miui.core", 
    "com.miui.daemon", 
    "com.miui.analytics",
    "com.xiaomi.joyose",            
    "com.miui.systemAdSolution"
  };
     
  // Iterate through the list of apps.
  for (const string& app : apps) {
    // Use exec_shell() function to execute shell command.
    exec_shell("pm disable " + app, false);  
  }

  // Print tweak completion message.     
  xlog("date", "Disabled unnecessary MIUI apps.");  
}

// Tweak to enable unnecessary MIUI apps.
void enable_miui_apps() {
  // List of apps to be enabled.
  vector<string> apps = {
    "com.miui.core", 
    "com.miui.daemon", 
    "com.miui.analytics",
    "com.xiaomi.joyose",            
    "com.miui.systemAdSolution"
  };
     
  // Iterate through the list of apps.
  for (const string& app : apps) {
    // Use exec_shell() function to execute shell command.
    exec_shell("pm enable " + app, false);  
  }

  // Print tweak completion message.   
  xlog("date", "Disabled MIUI tweaks. Reboot to fully revert the changes but it is optional.");      
}

void miui_props_tweak() {
  // List of properties to be modified.
  vector<string> props = {
    "persist.sys.miui.sf_cores 6", 
    "persist.sys.miui_animator_sched.bigcores 6-7",
    "persist.sys.enable_miui_booster 0"    
  };
  
  // Iterate through the list of properties.
  for (const string& prop : props) {
    // Use exec_shell() function to execute shell command.
    exec_shell("resetprop " + prop, false);  
  }

  // Print tweak completion message.   
  xlog("date", "Tuned MIUI props.");  
}

// Tweak to disable unnecessary MIUI services.
void disable_unnecessary_miui_services() {
  // Use kill_process() function to kill process.
  kill_process("miuibooster");

  // Print tweak completion message.   
  xlog("date", "Turned off unnecessary MIUI services.");    
}

void apply_all_tweaks() {
  xlog("info", "Started Android Enhancer Tweaks at " + print_date("full"));
  xlog("", "");
  
  kernel_tweak(); 
  io_tweak();  
  priority_tweak();     
  cmd_services_tweak();  
  disable_unnecessary_services();  
  props_tweak(); 
  dalvik_props_tweak();
  main_dex_tweak();  
  secondary_dex_tweak();  
  clean_junk();    
  enable_mem_preload_tweak();

  // Apply the following tweaks if MIUI is installed.
  if (is_app_exists("com.xiaomi.misettings")) {
    disable_miui_apps(); 
    miui_props_tweak();
    disable_unnecessary_miui_services();
  }

  xlog("", "");
  xlog("info", "Completed Android Enhancer Tweaks at " + print_date("full"));
}

void apply_main_tweaks() {
  xlog("info", "Started main tweaks at " + print_date("full"));
  
  kernel_tweak(); 
  io_tweak();
  priority_tweak();  
  cmd_services_tweak();   
  disable_unnecessary_services();  
  props_tweak();  
  
  xlog("info", "Completed main tweaks at " + print_date("full"));
}

void apply_miui_tweaks() {
  xlog("info", "Started MIUI tweaks at " + print_date("full"));
  
  disable_miui_apps(); 
  miui_props_tweak();
  disable_unnecessary_miui_services();
    
  xlog("info", "Completed MIUI tweaks at " + print_date("full"));
}

void apply_dalvik_tweaks() {
  xlog("info", "Started dalvik tweaks at " + print_date("full"));
  
  dalvik_props_tweak();
  main_dex_tweak();  
  secondary_dex_tweak();
    
  xlog("info", "Completed dalvik tweaks at " + print_date("full"));
}

int main(int argc, char *argv[]) {
  // Write all modified data to disk.
  sync();
  
  // Get command line argument.
  if (argv[1] == nullptr) {  
    xlog("error", "No command line argument provided. Exiting.");
    exit(1); 
  }
  string flag = argv[1]; 
  
  // Check for matching flag then perform respective operation.
  if (flag == "--apply-all-tweaks") {
    apply_all_tweaks();  
  } else if (flag == "--main-tweaks") {
    apply_main_tweaks();  
  } else if (flag == "--dalvik-tweaks") {
    apply_dalvik_tweaks();    
  } else if (flag == "--clean-junk") {
    clean_junk();    
  } else if (flag == "--enable-mem-preload-tweak") {
    enable_mem_preload_tweak();
  } else if (flag == "--disable-mem-preload-tweak") {
    disable_mem_preload_tweak();
  } else if (flag == "--enable-miui-tweaks") {
    apply_miui_tweaks();  
  } else if (flag == "--disable-miui-tweaks") {
    enable_miui_apps();    
  }
  
  // Write all modified data to disk.
  sync();
  
  return 0;
}