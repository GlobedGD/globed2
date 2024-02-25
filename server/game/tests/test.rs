// this doc is mostly for flamegraphs
#![allow(clippy::wildcard_imports, clippy::cast_possible_truncation)]
use esp::{ByteBuffer, ByteReader};
use globed_game_server::{data::*, managers::LevelManager};
use std::hint::black_box;

const ITERS: usize = 500_000;

#[test]
fn test_alloca_buffer() {
    let data = PlayerAccountData {
        account_id: 234_234_234,
        user_id: 234_234_234,
        name: FastString::from_str("hit his is my name"),
        icons: PlayerIconData::default(),
        special_user_data: Some(SpecialUserData {
            name_color: Color3B { r: 10, g: 100, b: 200 },
        }),
    };

    for _ in 0..ITERS {
        let func1 = black_box(|| {
            alloca::with_alloca(PlayerAccountData::ENCODED_SIZE * 64, |data_| {
                let stackarray = unsafe {
                    let ptr = data_.as_mut_ptr().cast::<u8>();
                    std::slice::from_raw_parts_mut(ptr, std::mem::size_of_val(data_))
                };

                let mut buf = FastByteBuffer::new(stackarray);
                for _ in 0..64 {
                    buf.write_value(&data);
                }

                let mut buf = ByteReader::from_bytes(stackarray);
                for _ in 0..64 {
                    assert_eq!(buf.read_value::<PlayerAccountData>().unwrap().account_id, data.account_id);
                }
            });
        });

        func1();
    }
}

#[test]
fn test_fast_buffer() {
    let data = PlayerAccountData {
        account_id: 234_234_234,
        user_id: 234_234_234,
        name: FastString::from_str("hit his is my name"),
        icons: PlayerIconData::default(),
        special_user_data: Some(SpecialUserData {
            name_color: Color3B { r: 10, g: 100, b: 200 },
        }),
    };

    for _ in 0..ITERS {
        let func1 = black_box(|| {
            let mut stackarray = [0u8; PlayerAccountData::ENCODED_SIZE * 64];
            let mut buf = FastByteBuffer::new(&mut stackarray);
            for _ in 0..64 {
                buf.write_value(&data);
            }

            let mut buf = ByteReader::from_bytes(&stackarray);
            for _ in 0..64 {
                assert_eq!(buf.read_value::<PlayerAccountData>().unwrap().account_id, data.account_id);
            }
        });

        func1();
    }
}

#[test]
fn test_slow_buffer() {
    let data = PlayerAccountData {
        account_id: 234_234_234,
        user_id: 234_234_234,
        name: FastString::from_str("hit his is my name"),
        icons: PlayerIconData::default(),
        special_user_data: Some(SpecialUserData {
            name_color: Color3B { r: 10, g: 100, b: 200 },
        }),
    };

    for _ in 0..ITERS {
        let func2 = black_box(|| {
            let mut buffer = ByteBuffer::new();
            for _ in 0..64 {
                buffer.write_value(&data);
            }

            let mut reader = ByteReader::from_bytes(buffer.as_bytes());

            for _ in 0..64 {
                assert_eq!(reader.read_value::<PlayerAccountData>().unwrap().account_id, data.account_id);
            }
        });

        func2();
    }
}

#[test]
fn test_player_manager() {
    let mut manager = LevelManager::new();

    for level_id in 0..100 {
        for account_id in 0..100 {
            manager.add_to_level(level_id, level_id as i32 * 100 + account_id);
            manager.set_player_data(level_id as i32 * 100 + account_id, &PlayerData::default());
        }
    }

    let mut total_players = 0;
    for i in 0..100 {
        let count = manager.get_player_count_on_level(i).unwrap_or(0);
        assert_eq!(count, 100);

        let total = manager.for_each_player_on_level(
            i,
            |_, _, p| {
                *p += 1;
                true
            },
            &mut total_players,
        );

        assert_eq!(total, count);
    }

    assert_eq!(total_players, 10000);

    for level_id in 0..100 {
        for account_id in 0..100 {
            manager.remove_from_level(level_id, level_id as i32 * 100 + account_id);
            manager.remove_player(level_id as i32 * 100 + account_id);
        }
    }
}
