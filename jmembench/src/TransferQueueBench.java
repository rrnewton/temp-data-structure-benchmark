import java.util.Arrays;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedTransferQueue;


public class TransferQueueBench {
	protected final LinkedTransferQueue<Long> queue;
	private final ExecutorService pool;
	
	public TransferQueueBench() {
		queue = new LinkedTransferQueue<Long>();
		pool = Executors.newFixedThreadPool(2);
	}
	
	public void bench(long elements) {
		try {
			pool.invokeAll(Arrays.asList(new Producer(elements, queue), 
					new Consumer(elements, queue)));
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	
	public void finalize() {
		assert(pool.shutdownNow().isEmpty());
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		TransferQueueBench q = new TransferQueueBench();
		long n = Long.parseLong(args[0]);
		
		long start = System.currentTimeMillis();
		q.bench(n);
		long end = System.currentTimeMillis();
		
		double secs = (end-start) / 1000.0;
		double mbs = n*8*2*1e-06;
		System.out.println((mbs/secs) + "MB/s");
		
		q.finalize();
	}

	private class Producer implements Callable<Boolean> {
		private long n;
		private LinkedTransferQueue<Long> queue;
		
		public Producer(long n, LinkedTransferQueue<Long> queue) {
			this.n = n;
			this.queue = queue;
		}
		
		public Boolean call() {
			for (long i = 0; i < n; i++)
				queue.add(i);
			return true;
		}
	}
	
	private class Consumer implements Callable<Boolean> {
		private long n;
		private LinkedTransferQueue<Long> queue;
		
		public Consumer(long n, LinkedTransferQueue<Long> queue) {
			this.n = n;
			this.queue = queue;
		}
		
		public Boolean call() {
			for (long i = 0; i < n; i++)
				try {
					queue.take();
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			return true;
		}
	}
}
