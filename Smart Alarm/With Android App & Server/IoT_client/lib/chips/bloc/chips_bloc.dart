import 'dart:async';

import 'package:bloc/bloc.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:injectable/injectable.dart';
import 'package:iot_client/core/chip.dart';
import 'package:iot_client/core/chips_repository.dart';

part 'chips_event.dart';
part 'chips_state.dart';
part 'chips_bloc.freezed.dart';

@injectable
class ChipsBloc extends Bloc<ChipsEvent, ChipsState> {
  final ChipsRepository _repository;

  ChipsBloc(this._repository) : super(const ChipsState.loadInProgress());

  @override
  Stream<ChipsState> mapEventToState(ChipsEvent event) async* {
    yield* event.map(
      loaded: (e) async* {
        yield const ChipsState.loadInProgress();

        final result = await _repository.getAll();

        yield result.fold(
          (_) => const ChipsState.loadFailure(),
          (chips) => ChipsState.loadSuccess(chips.reversed.toList()),
        );
      },
    );
  }
}
