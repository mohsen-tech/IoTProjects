import 'package:dio/dio.dart';
import 'package:injectable/injectable.dart';

@module
abstract class InjectableModules {
  @singleton
  Dio dioClient() {
    return Dio(
      BaseOptions(
        baseUrl: const String.fromEnvironment('BASE_URL'),
        connectTimeout: 10000,
        responseType: ResponseType.json,
      ),
    )..interceptors.add(
        LogInterceptor(
          error: true,
          request: true,
          requestBody: false,
          requestHeader: false,
          responseBody: true,
          responseHeader: false,
        ),
      );
  }
}
