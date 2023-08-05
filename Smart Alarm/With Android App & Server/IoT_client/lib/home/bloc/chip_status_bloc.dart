import 'dart:async';

import 'package:bloc/bloc.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:injectable/injectable.dart';
import 'package:iot_client/core/chips_repository.dart';
import 'package:iot_client/core/local_notifications.dart';

part 'chip_status_event.dart';
part 'chip_status_state.dart';
part 'chip_status_bloc.freezed.dart';

@singleton
class ChipStatusBloc extends Bloc<ChipStatusEvent, ChipStatusState> {
  final ChipsRepository _repository;
  final LocalNotifications _localNotifications;
  Timer? _timer;

  ChipStatusBloc(this._repository, this._localNotifications)
      : super(const ChipStatusState.loadInProgress()) {
    _startTimer();
  }

  void _startTimer() {
    _timer = Timer.periodic(const Duration(seconds: 3), (timer) {
      add(const ChipStatusEvent.loaded());
    });
  }

  @override
  Future<void> close() {
    _timer?.cancel();
    return super.close();
  }

  @override
  Stream<ChipStatusState> mapEventToState(ChipStatusEvent event) async* {
    yield* event.map(
      loaded: (e) async* {
        yield const ChipStatusState.loadInProgress();

        final result = await _repository.getStatus();

        yield* result.fold(
          (_) async* {
            yield const ChipStatusState.loadFailure();
          },
          (status) async* {
            if (status) await _localNotifications.show();
            yield ChipStatusState.loadSuccess(status);
          },
        );
      },
    );
  }
}
