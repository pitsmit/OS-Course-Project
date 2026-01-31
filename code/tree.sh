cd /sys/kernel/debug/tracing

# Очистить текущую конфигурацию
echo 0 > tracing_on
echo nop > current_tracer
echo > trace
echo > set_ftrace_filter
echo > set_graph_function

echo "*fat*" > set_ftrace_filter
#echo "*usb*" >> set_ftrace_filter
#echo "*urb*" >> set_ftrace_filter
#echo "*hcd*" >> set_ftrace_filter
#echo "*xhci*" >> set_ftrace_filter

echo function_graph > current_tracer

# Включить трассировку
echo 1 > tracing_on
echo > trace
cd /home/pitsmit/KURSACH/code
cp /home/pitsmit/KURSACH/code/Readme.md "/media/pitsmit/MYFLASH" &
CP_PID=$!

echo $CP_PID > set_ftrace_pid
wait $CP_PID
sync

sleep 1
cd /sys/kernel/debug/tracing
echo 0 > tracing_on
cat trace | tail -1000 > /tmp/tree-2.txt
cp /tmp/tree-2.txt /home/pitsmit/KURSACH/code