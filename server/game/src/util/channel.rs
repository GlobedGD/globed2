use crate::tokio::sync::mpsc;

use super::LockfreeMutCell;

/// Simple wrapper around `tokio::mpsc` channels except receiver does not need to be mutable.
/// Obviously not safe to call `recv` from multiple threads, it's a single consumer channel duh
pub struct TokioChannel<T> {
    pub tx: mpsc::Sender<T>,
    pub rx: LockfreeMutCell<mpsc::Receiver<T>>,
}

pub struct SenderDropped;

impl<T> TokioChannel<T> {
    pub fn new(size: usize) -> Self {
        Self::from_tx_rx(mpsc::channel(size))
    }

    fn from_tx_rx((tx, rx): (mpsc::Sender<T>, mpsc::Receiver<T>)) -> Self {
        Self {
            tx,
            rx: LockfreeMutCell::new(rx),
        }
    }

    pub fn try_send(&self, msg: T) -> Result<(), mpsc::error::TrySendError<T>> {
        self.tx.try_send(msg)
    }

    pub async fn send(&self, msg: T) -> Result<(), mpsc::error::SendError<T>> {
        self.tx.send(msg).await
    }

    /// Safety: is guaranteed to be safe as long as you don't call it from multiple threads at once.
    pub async unsafe fn recv(&self) -> Result<T, SenderDropped> {
        let chan = self.rx.get_mut();
        chan.recv().await.ok_or(SenderDropped)
    }
}
/*
* Supposedly, those two should be faster than tokio::mpsc but I noticed no real difference in my benchmarking.
* Feel free to install crates `loole` and `kanal` and try those channels yourself if you want.
 */

// /// Channel significantly faster than `tokio::mpsc` (and by relation, `UnsafeChannel`)
// pub struct FastChannel<T> {
//     pub tx: kanal::AsyncSender<T>,
//     pub rx: kanal::AsyncReceiver<T>,
// }

// impl<T> FastChannel<T> {
//     pub fn new_bounded(size: usize) -> Self {
//         Self::from_tx_rx(kanal::bounded_async(size))
//     }

//     pub fn new_unbounded() -> Self {
//         Self::from_tx_rx(kanal::unbounded_async())
//     }

//     fn from_tx_rx((tx, rx): (kanal::AsyncSender<T>, kanal::AsyncReceiver<T>)) -> Self {
//         Self { tx, rx }
//     }

//     pub fn try_send(&self, msg: T) -> Result<bool, kanal::SendError> {
//         self.tx.try_send(msg)
//     }

//     pub async fn send(&self, msg: T) -> Result<(), kanal::SendError> {
//         self.tx.send(msg).await
//     }

//     pub async fn recv(&self) -> Result<T, kanal::ReceiveError> {
//         self.rx.recv().await
//     }
// }

// pub struct LooleChannel<T> {
//     pub tx: loole::Sender<T>,
//     pub rx: loole::Receiver<T>,
// }

// impl<T> LooleChannel<T> {
//     pub fn new_bounded(size: usize) -> Self {
//         Self::from_tx_rx(loole::bounded(size))
//     }

//     pub fn new_unbounded() -> Self {
//         Self::from_tx_rx(loole::unbounded())
//     }

//     pub fn try_send(&self, msg: T) -> Result<(), loole::TrySendError<T>> {
//         self.tx.try_send(msg)
//     }

//     pub async fn send(&self, msg: T) -> Result<(), loole::SendError<T>> {
//         self.tx.send_async(msg).await
//     }

//     pub async fn recv(&self) -> Result<T, loole::RecvError> {
//         self.rx.recv_async().await
//     }

//     fn from_tx_rx((tx, rx): (loole::Sender<T>, loole::Receiver<T>)) -> Self {
//         Self { tx, rx }
//     }
// }
