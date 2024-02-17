use diesel::SqliteConnection;
use diesel_migrations::{embed_migrations, EmbeddedMigrations, MigrationHarness};
use globed_shared::anyhow::{self, anyhow};
use rocket_sync_db_pools::database;

pub const MIGRATIONS: EmbeddedMigrations = embed_migrations!("migrations");

pub async fn run_migrations(db: &GlobedDb) -> anyhow::Result<()> {
    // run migrations
    // stupid rust does not allow me to use the ? operator here
    let result = db.run(|conn| conn.run_pending_migrations(MIGRATIONS).map(|_| ())).await;

    match result {
        Ok(()) => {}
        Err(e) => return Err(anyhow!("{e}")),
    }

    Ok(())
}

#[database("globed_db")]
pub struct GlobedDb(SqliteConnection);
