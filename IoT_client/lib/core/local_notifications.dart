import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:injectable/injectable.dart';

@lazySingleton
class LocalNotifications {
  final _flutterLocalNotificationsPlugin = FlutterLocalNotificationsPlugin();
  bool _isInitialized = false;

  Future<void> _initialize() async {
    const initializationSettingsAndroid =
        AndroidInitializationSettings('warning');
    const initializationSettings =
        InitializationSettings(android: initializationSettingsAndroid);
    await _flutterLocalNotificationsPlugin.initialize(initializationSettings);
    _isInitialized = true;
  }

  Future<void> show() async {
    if (_isInitialized == false) await _initialize();
    const androidPlatformChannelSpecifics = AndroidNotificationDetails(
      'iot_project',
      'iot_project',
      'iot_project',
      importance: Importance.max,
      priority: Priority.high,
      showWhen: true,
      autoCancel: true,
      channelShowBadge: true,
      enableLights: true,
      enableVibration: true,
      playSound: true,
      sound: RawResourceAndroidNotificationSound('woop_woop'),
    );
    const platformChannelSpecifics =
        NotificationDetails(android: androidPlatformChannelSpecifics);
    await _flutterLocalNotificationsPlugin.show(
      DateTime.now().millisecondsSinceEpoch ~/ 1000,
      'Warning',
      'Mes ke ye khabarie :D',
      platformChannelSpecifics,
    );
  }
}
