#![allow(clippy::wildcard_imports, clippy::cast_possible_truncation)]
use criterion::{Criterion, black_box, criterion_group, criterion_main};
use esp::{ByteBuffer, ByteReader};
use globed_game_server::{data::*, make_uninit, managers::LevelManager, new_uninit};
use globed_shared::{
    generate_alphanum_string,
    rand::{self, Rng, RngCore},
};

fn buffers(c: &mut Criterion) {
    let data = PlayerAccountData {
        account_id: 234_234_234,
        user_id: 234_234_234,
        name: InlineString::new("hit his is my name"),
        icons: PlayerIconData::default(),
        special_user_data: SpecialUserData { roles: None },
    };

    c.bench_function("alloca-byte-buffer", |b| {
        b.iter(black_box(|| {
            alloca::with_alloca(PlayerAccountData::ENCODED_SIZE * 64, |data_| {
                let stackarray = unsafe { make_uninit!(data_) };

                let mut buf = FastByteBuffer::new(stackarray);
                for _ in 0..64 {
                    buf.write_value(&data);
                }

                // let mut buf = ByteReader::from_bytes(stackarray);
                // for _ in 0..64 {
                //     assert_eq!(buf.read_value::<PlayerAccountData>().unwrap().account_id, data.account_id);
                // }
            });
        }));
    });

    c.bench_function("fast-byte-buffer", |b| {
        b.iter(black_box(|| {
            let mut stackarray = [0u8; PlayerAccountData::ENCODED_SIZE * 64];
            let mut buf = FastByteBuffer::new(&mut stackarray);
            for _ in 0..64 {
                buf.write_value(&data);
            }

            // let mut buf = ByteReader::from_bytes(&stackarray);
            // for _ in 0..64 {
            //     assert_eq!(buf.read_value::<PlayerAccountData>().unwrap().account_id, data.account_id);
            // }
        }));
    });

    c.bench_function("fast-byte-buffer-uninit", |b| {
        b.iter(black_box(|| {
            let stackarray = unsafe { new_uninit!(PlayerAccountData::ENCODED_SIZE * 64) };
            let mut buf = FastByteBuffer::new(stackarray);
            for _ in 0..64 {
                buf.write_value(&data);
            }

            // let mut buf = ByteReader::from_bytes(&stackarray);
            // for _ in 0..64 {
            //     assert_eq!(buf.read_value::<PlayerAccountData>().unwrap().account_id, data.account_id);
            // }
        }));
    });

    c.bench_function("slow-byte-buffer", |b| {
        b.iter(black_box(|| {
            let mut buffer = ByteBuffer::new();
            for _ in 0..64 {
                buffer.write_value(&data);
            }
        }));
    });
}

fn structs(c: &mut Criterion) {
    let mut data = [0u8; 2048];
    rand::thread_rng().fill_bytes(&mut data);

    c.bench_function("encode-audio-frame", |b| {
        b.iter(black_box(|| {
            let data = FastEncodedAudioFrame { data: data.to_vec().into() };
            let mut stack_array = [0u8; 3000 * 8];
            let mut buf = FastByteBuffer::new(&mut stack_array);
            for _ in 0..8 {
                buf.write_value(&data);
            }

            let written_data = buf.as_bytes();

            for _ in 0..128 {
                let mut reader = ByteReader::from_bytes(written_data);

                let abc = reader.read_value::<FastEncodedAudioFrame>().unwrap();
                assert_eq!(abc.data.len(), written_data.len());
            }
        }));
    });
}

fn managers(c: &mut Criterion) {
    c.bench_function("player-manager", |b| {
        b.iter(black_box(|| {
            let mut manager = LevelManager::new();

            for level_id in 0..100 {
                for account_id in 0..10 {
                    manager.add_to_level(level_id, level_id as i32 * 10 + account_id, false);
                    manager.set_player_data(level_id as i32 * 10 + account_id, &PlayerData::default());
                }
            }

            let mut total_players = 0;
            for i in 0..100 {
                let count = manager.get_player_count_on_level(i).unwrap_or(0);
                assert_eq!(count, 10);

                manager.for_each_player_on_level(i, |_| {
                    total_players += 1;
                });

                assert_eq!(total_players, count);
            }

            assert_eq!(total_players, 1000);

            for level_id in 0..100 {
                for account_id in 0..10 {
                    manager.remove_from_level(level_id, level_id as i32 * 10 + account_id);
                    manager.remove_player(level_id as i32 * 10 + account_id);
                }
            }
        }));
    });
}

fn read_value_array(c: &mut Criterion) {
    c.bench_function("read-value-array", |b| {
        let mut buf = ByteBuffer::new();
        for _ in 0..1024 {
            buf.write_value_array(&black_box([69u8; 128]));
        }

        b.iter(black_box(|| {
            buf.set_rpos(0);
            for _ in 0..1024 {
                let v = black_box(buf.read_value_array::<u8, 128>()).unwrap();
                assert_eq!(v[0], v[127]);
                assert_eq!(v[0], 69u8);
            }
        }));
    });
}

fn strings(c: &mut Criterion) {
    let mut buf_short = ByteBuffer::new();
    for _ in 0..1024 {
        buf_short.write_value(&generate_alphanum_string(rand::thread_rng().gen_range(8..32)));
    }

    let mut buf_med = ByteBuffer::new();
    for _ in 0..1024 {
        buf_med.write_value(&generate_alphanum_string(rand::thread_rng().gen_range(56..128)));
    }

    let mut buf_long = ByteBuffer::new();
    for _ in 0..1024 {
        buf_long.write_value(&generate_alphanum_string(rand::thread_rng().gen_range(164..512)));
    }

    let mut output = ByteBuffer::with_capacity(buf_long.len() + 1024);

    c.bench_function("read-string-short", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_short.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<String>()).unwrap();
                // output.write_value(&str);
            }
        });
    });

    c.bench_function("read-fast-string-short", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_short.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<FastString>()).unwrap();
                // output.write_value(&str);
            }
        });
    });

    c.bench_function("read-inline-string-short", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_short.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<InlineString<32>>()).unwrap();
                // output.write_value(&str);
            }
        });
    });

    c.bench_function("read-string-medium", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_med.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<String>()).unwrap();
                // output.write_value(&str);
            }
        });
    });

    c.bench_function("read-fast-string-medium", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_med.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<FastString>()).unwrap();
                // output.write_value(&str);
            }
        });
    });

    c.bench_function("read-string-long", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_long.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<String>()).unwrap();
                // output.write_value(&str);
            }
        });
    });

    c.bench_function("read-fast-string-long", |b| {
        b.iter(|| {
            // output.clear();
            let mut reader = ByteReader::from_bytes(buf_long.as_bytes());
            for _ in 0..1024 {
                let str = black_box(reader.read::<FastString>()).unwrap();
                // output.write_value(&str);
            }
        });
    });
}

// criterion_group!(benches, buffers, structs, managers, read_value_array, strings);
criterion_group!(benches, strings);
criterion_main!(benches);
