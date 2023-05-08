import 'package:dartz/dartz.dart';
import 'package:dio/dio.dart';
import 'package:injectable/injectable.dart';
import 'package:iot_client/core/chip.dart';

class ChipsFailure {
  const ChipsFailure();
}

@singleton
class ChipsRepository {
  final Dio _dio;

  const ChipsRepository(this._dio);

  Future<Either<ChipsFailure, List<Chip>>> getAll() async {
    try {
      final result = await _dio.get('/list');

      return Right(
        (result.data as List)
            .cast<Map<String, dynamic>>()
            .map((json) => Chip.fromJson(json))
            .toList(),
      );
    } catch (e) {
      return const Left(ChipsFailure());
    }
  }

  Future<Either<ChipsFailure, bool>> getStatus() async {
    try {
      final result = await _dio.get('/status');

      return Right(result.data['status']);
    } catch (e) {
      return const Left(ChipsFailure());
    }
  }
}
