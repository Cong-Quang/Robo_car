import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';

/// Dịch vụ UDP gửi dữ liệu dạng binary (6 bytes: 2 byte cho j1X, 2 byte cho j1Y, 2 byte cho speed)
class UdpService {
  RawDatagramSocket? _socket;
  InternetAddress _targetAddress = InternetAddress('192.168.1.100');
  int _port = 65000;

  // Callback để gửi thông báo log lên giao diện
  Function(String)? _logCallback;

  UdpService({Function(String)? logCallback}) {
    _logCallback = logCallback;
  }

  Future<void> init() async {
    try {
      _socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      _logCallback?.call('UDP socket initialized');
    } catch (e) {
      _logCallback?.call('Failed to initialize UDP socket: $e');
    }
  }

  void updateTarget(String ip, int port) {
    try {
      _targetAddress = InternetAddress(ip);
      _port = port;
      _logCallback?.call('Updated UDP target: IP=$_targetAddress, Port=$_port');
    } catch (e) {
      _logCallback?.call('Invalid IP or Port: $e');
    }
  }

  // Hàm gửi dữ liệu dạng byte
  void sendData(Uint8List data) {
    if (_socket != null) {
      try {
        _socket!.send(data, _targetAddress, _port);
        _logCallback?.call('Sent UDP binary data: $data to $_targetAddress:$_port');
      } catch (e) {
        _logCallback?.call('Failed to send UDP data: $e');
      }
    } else {
      _logCallback?.call('Socket is not initialized');
    }
  }

  void close() {
    _socket?.close();
    _socket = null;
    _logCallback?.call('UDP socket closed');
  }
}

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft,
    DeviceOrientation.landscapeRight,
  ]).then((_) {
    runApp(const MyApp());
  });
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'UDP Controller',
      debugShowCheckedModeBanner: false,
      home: const SettingsPage(),
    );
  }
}

/// Trang cài đặt để nhập địa chỉ IP, cổng UDP và cài đặt Camera
class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});
  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late final UdpService _udpService;
  final TextEditingController _ipController =
  TextEditingController(text: '192.168.1.100');
  final TextEditingController _portController =
  TextEditingController(text: '65000');

  // Controller cho camera
  final TextEditingController _cameraIpController =
  TextEditingController(text: '192.168.1.101');
  final TextEditingController _cameraPortController =
  TextEditingController(text: '2003');

  @override
  void initState() {
    super.initState();
    _udpService = UdpService(logCallback: (log) {
      debugPrint(log);
    });
    _udpService.init();
  }

  @override
  void dispose() {
    _ipController.dispose();
    _portController.dispose();
    _cameraIpController.dispose();
    _cameraPortController.dispose();
    super.dispose();
  }

  void _saveUdpSettings() {
    String ip = _ipController.text;
    int port = int.tryParse(_portController.text) ?? 65000;
    _udpService.updateTarget(ip, port);

    // Lấy thông tin camera
    String camIp = _cameraIpController.text;
    int camPort = int.tryParse(_cameraPortController.text) ?? 80;
    debugPrint('Camera settings: IP=$camIp, Port=$camPort');

    Navigator.pushReplacement(
      context,
      MaterialPageRoute(
        builder: (context) => ControlPage(
          udpService: _udpService,
          cameraIp: camIp,
          cameraPort: camPort,
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    // Sử dụng SingleChildScrollView để tránh bottom overflow khi bàn phím xuất hiện.
    return Scaffold(
      appBar: AppBar(title: const Text('Cài đặt')),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20.0),
        child: ConstrainedBox(
          constraints: BoxConstraints(
            minHeight: MediaQuery.of(context).size.height -
                kToolbarHeight -
                MediaQuery.of(context).padding.top,
          ),
          child: IntrinsicHeight(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // Hàng nhập liệu cho UDP: Địa chỉ IP và Cổng UDP
                Row(
                  children: [
                    Expanded(
                      child: TextField(
                        controller: _ipController,
                        decoration:
                        const InputDecoration(labelText: 'Địa chỉ IP'),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: TextField(
                        controller: _portController,
                        decoration:
                        const InputDecoration(labelText: 'Cổng UDP'),
                        keyboardType: TextInputType.number,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 20),
                // Hàng nhập liệu cho Camera: Địa chỉ Camera và Cổng Camera
                Row(
                  children: [
                    Expanded(
                      child: TextField(
                        controller: _cameraIpController,
                        decoration: const InputDecoration(
                            labelText: 'Địa chỉ Camera'),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: TextField(
                        controller: _cameraPortController,
                        decoration: const InputDecoration(
                            labelText: 'Cổng Camera'),
                        keyboardType: TextInputType.number,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 20),
                ElevatedButton(
                  onPressed: _saveUdpSettings,
                  child: const Text('Lưu & Điều khiển'),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

/// Trang điều khiển sử dụng Joystick để gửi dữ liệu UDP dạng binary
/// Đồng thời hiển thị video stream từ ESP32-CAM.
class ControlPage extends StatefulWidget {
  final UdpService udpService;
  final String cameraIp;
  final int cameraPort;
  const ControlPage({
    super.key,
    required this.udpService,
    required this.cameraIp,
    required this.cameraPort,
  });

  @override
  State<ControlPage> createState() => _ControlPageState();
}

class _ControlPageState extends State<ControlPage> {
  // Tọa độ joystick (x: trái-phải, y: dưới-lên)
  double _joystick1X = 0.0;
  double _joystick1Y = 0.0;
  bool nitro = false;

  // Giới hạn log hiển thị là 2 dòng
  final List<String> _consoleLogs = [];
  final ScrollController _scrollController = ScrollController();
  static const int _maxLogs = 2;

  void _addLog(String log) {
    setState(() {
      if (_consoleLogs.length >= _maxLogs) {
        _consoleLogs.removeAt(0);
      }
      _consoleLogs.add(log);
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_scrollController.hasClients) {
          _scrollController.jumpTo(_scrollController.position.maxScrollExtent);
        }
      });
    });
  }

  /// Hàm gửi dữ liệu UDP dạng binary:
  /// Gói dữ liệu gồm 6 bytes: 2 byte cho j1X, 2 byte cho j1Y, 2 byte cho speed (nitro).
  void _sendUdpData() {
    int j1X = (_joystick1X * 100).round();
    int j1Y = (-_joystick1Y * 100).round();
    int speed = nitro ? 100 : 0;

    ByteData bd = ByteData(6);
    bd.setInt16(0, j1X, Endian.little);
    bd.setInt16(2, j1Y, Endian.little);
    bd.setInt16(4, speed, Endian.little);

    Uint8List data = bd.buffer.asUint8List();
    widget.udpService.sendData(data);
    _addLog('j1X=$j1X, j1Y=$j1Y, spd=$speed');
  }

  // Khi nhấn giữ nút Nitro: bật nitro và gửi dữ liệu UDP
  void _onNitroPress() {
    setState(() {
      nitro = true;
    });
    _addLog('Nitro on');
    _sendUdpData();
  }

  // Khi nhả nút Nitro: tắt nitro và gửi dữ liệu UDP
  void _onNitroRelease() {
    setState(() {
      nitro = false;
    });
    _addLog('Nitro off');
    _sendUdpData();
  }

  @override
  void dispose() {
    widget.udpService.close();
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      // Thu nhỏ AppBar xuống 50% so với chiều cao mặc định
      appBar: PreferredSize(
        preferredSize: const Size.fromHeight(28),
        child: AppBar(
          title: const Text('Điều khiển', style: TextStyle(fontSize: 16)),
          actions: [
            IconButton(
              icon: const Icon(Icons.settings, size: 16),
              tooltip: 'Quay lại cài đặt',
              onPressed: () {
                Navigator.pushReplacement(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const SettingsPage(),
                  ),
                );
              },
            ),
          ],
        ),
      ),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Colors.blueGrey, Colors.black],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: Stack(
          children: [
            // Video stream được căn giữa màn hình và dịch sang bên phải (offset 30)
            Center(
              child: Transform.translate(
                offset: const Offset(30, 0),
                child: Container(
                  width: 300,
                  height: 300,
                  decoration: BoxDecoration(
                    border: Border.all(color: Colors.white, width: 2),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Mjpeg(
                    stream:
                    'http://${widget.cameraIp}:${widget.cameraPort}/stream',
                    isLive: true,
                    error: (context, error, stack) {
                      return Center(
                        child: Text(
                          'Lỗi tải video: $error',
                          style: const TextStyle(color: Colors.red),
                        ),
                      );
                    },
                  ),
                ),
              ),
            ),
            // Container chứa Joystick ở góc trái dưới màn hình
            Positioned(
              left: 20,
              bottom: 20,
              child: Container(
                padding: const EdgeInsets.all(8.0),
                decoration: BoxDecoration(
                  color: Colors.white24,
                  borderRadius: BorderRadius.circular(10),
                ),
                child: Joystick(
                  mode: JoystickMode.all,
                  listener: (details) {
                    setState(() {
                      _joystick1X = details.x;
                      _joystick1Y = details.y;
                    });
                    _sendUdpData();
                  },
                ),
              ),
            ),
            // Console log hiển thị ở phía trên màn hình với width cố định (250px)
            Positioned(
              top: 10,
              left: 10,
              child: SizedBox(
                width: 250,
                child: ConsoleLogWidget(
                  logs: _consoleLogs,
                  controller: _scrollController,
                ),
              ),
            ),
            // Nút tròn "Nitro" hiển thị ở góc dưới bên phải màn hình với cơ chế giữ
            Positioned(
              right: 20,
              bottom: 20,
              child: GestureDetector(
                onTapDown: (_) => _onNitroPress(),
                onTapUp: (_) => _onNitroRelease(),
                onTapCancel: () => _onNitroRelease(),
                child: FloatingActionButton(
                  backgroundColor: nitro ? Colors.red : Colors.green,
                  onPressed: () {},
                  child: const Text(
                    'Nitro',
                    textAlign: TextAlign.center,
                    style: TextStyle(fontSize: 16),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// Widget ConsoleLogWidget nhận danh sách log động và ScrollController từ ControlPage
class ConsoleLogWidget extends StatelessWidget {
  final List<String> logs;
  final ScrollController controller;
  const ConsoleLogWidget({
    super.key,
    required this.logs,
    required this.controller,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
      decoration: BoxDecoration(
        color: Colors.black54,
        borderRadius: BorderRadius.circular(10),
      ),
      child: ListView.builder(
        controller: controller,
        shrinkWrap: true,
        itemCount: logs.length,
        itemBuilder: (context, index) {
          return Text(
            logs[index],
            style: const TextStyle(color: Colors.white, fontSize: 12),
          );
        },
      ),
    );
  }
}
