// GENERATED CODE - DO NOT MODIFY BY HAND

// **************************************************************************
// InjectableConfigGenerator
// **************************************************************************

import 'package:dio/dio.dart' as _i6;
import 'package:get_it/get_it.dart' as _i1;
import 'package:injectable/injectable.dart' as _i2;
import 'package:iot_client/chips/bloc/chips_bloc.dart' as _i4;
import 'package:iot_client/core/chips_repository.dart' as _i5;
import 'package:iot_client/core/injectable_modules.dart' as _i8;
import 'package:iot_client/core/local_notifications.dart' as _i3;
import 'package:iot_client/home/bloc/chip_status_bloc.dart'
    as _i7; // ignore_for_file: unnecessary_lambdas

// ignore_for_file: lines_longer_than_80_chars
/// initializes the registration of provided dependencies inside of [GetIt]
_i1.GetIt $initGetIt(_i1.GetIt get,
    {String? environment, _i2.EnvironmentFilter? environmentFilter}) {
  final gh = _i2.GetItHelper(get, environment, environmentFilter);
  final injectableModules = _$InjectableModules();
  gh.lazySingleton<_i3.LocalNotifications>(() => _i3.LocalNotifications());
  gh.factory<_i4.ChipsBloc>(() => _i4.ChipsBloc(get<_i5.ChipsRepository>()));
  gh.singleton<_i6.Dio>(injectableModules.dioClient());
  gh.singleton<_i5.ChipsRepository>(_i5.ChipsRepository(get<_i6.Dio>()));
  gh.singleton<_i7.ChipStatusBloc>(_i7.ChipStatusBloc(
      get<_i5.ChipsRepository>(), get<_i3.LocalNotifications>()));
  return get;
}

class _$InjectableModules extends _i8.InjectableModules {}
