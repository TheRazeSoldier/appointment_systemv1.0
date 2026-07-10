import 'dotenv/config';
import express from 'express';
import { AlipaySdk } from 'alipay-sdk';

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

const MOCK = process.env.MOCK_MODE === 'true';
const ALIPAY_APP_ID = process.env.ALIPAY_APP_ID || 'mock_app_id';
const ALIPAY_PRIVATE_KEY = process.env.ALIPAY_PRIVATE_KEY || 'mock_private_key';
const ALIPAY_PUBLIC_KEY = process.env.ALIPAY_PUBLIC_KEY || 'mock_public_key';
const ALIPAY_GATEWAY = process.env.ALIPAY_GATEWAY || 'https://openapi-sandbox.dl.alipaydev.com/gateway.do';

let alipaySdk = null;
if (!MOCK) {
  if (!process.env.ALIPAY_APP_ID || !process.env.ALIPAY_PRIVATE_KEY || !process.env.ALIPAY_PUBLIC_KEY) {
    console.error('[FATAL] Missing required env: ALIPAY_APP_ID / ALIPAY_PRIVATE_KEY / ALIPAY_PUBLIC_KEY');
    process.exit(1);
  }
  alipaySdk = new AlipaySdk({
    appId: ALIPAY_APP_ID,
    privateKey: ALIPAY_PRIVATE_KEY,
    alipayPublicKey: ALIPAY_PUBLIC_KEY,
    gateway: ALIPAY_GATEWAY,
    signType: 'RSA2',
    keyType: 'PKCS8'
  });
} else {
  console.log('[MOCK] Running in mock mode -- no real Alipay credentials needed');
}

app.post('/gateway/pay', (req, res) => {
  try {
    const { out_trade_no, subject, total_amount, return_url, notify_url } = req.body;
    if (!out_trade_no || !subject || !total_amount || !return_url || !notify_url) {
      return res.status(400).json({ error: 'Missing required payment fields' });
    }

    if (MOCK) {
      const paymentHtml = `<form id="alipayForm" action="${return_url}" method="POST"><input type="hidden" name="out_trade_no" value="${out_trade_no}"><input type="hidden" name="trade_status" value="TRADE_SUCCESS"><button type="submit" style="padding:16px 32px;font-size:1.2rem;background:#1677ff;color:#fff;border:none;border-radius:8px;cursor:pointer;">✔ 模拟支付成功，点击返回</button></form><script>document.getElementById('alipayForm').submit();</script>`;
      return res.json({ paymentHtml, out_trade_no, mock: true });
    }

    const formData = {
      returnUrl: return_url,
      notifyUrl: notify_url,
      bizContent: {
        out_trade_no,
        total_amount: String(total_amount),
        subject: 'test',
        product_code: 'FAST_INSTANT_TRADE_PAY'
      }
    };
    try {
      const paymentHtml = alipaySdk.pageExecute('alipay.trade.page.pay', 'POST', formData);
      res.json({ paymentHtml, out_trade_no });
    } catch (e) {
      res.status(500).json({ error: e.message || 'SDK error' });
    }
  } catch (error) {
    res.status(500).json({ error: error.message || 'Failed to create Alipay form' });
  }
  });

  app.post('/gateway/direct-pay', async (req, res) => {
    try {
      const { out_trade_no, total_amount, subject } = req.body;
      const result = await alipaySdk.exec('alipay.trade.page.pay', {
        returnUrl: req.body.return_url,
        notifyUrl: req.body.notify_url,
        bizContent: {
          out_trade_no,
          total_amount: String(total_amount),
          subject: subject || 'payment',
          product_code: 'FAST_INSTANT_TRADE_PAY'
        }
      });
      res.json(result);
    } catch (e) {
      console.error('[DirectPay]', e);
      res.status(500).json({ error: e.message || 'Direct pay failed', detail: e.response?.data ? String(e.response.data).substring(0,500) : 'no detail', code: e.code });
    }
});

app.post('/gateway/query', async (req, res) => {
  try {
    const { out_trade_no } = req.body;
    if (!out_trade_no) {
      return res.status(400).json({ error: 'Missing out_trade_no' });
    }

    if (MOCK) {
      return res.json({ out_trade_no, trade_status: 'TRADE_SUCCESS', total_amount: '0.01', send_pay_date: new Date().toISOString() });
    }

    const result = await alipaySdk.exec('alipay.trade.query', {
      bizContent: {
        out_trade_no,
        query_options: ['trade_settle_info']
      }
    });

    const payload = result.alipay_trade_query_response || result;
    if (!payload || payload.code !== '10000') {
      return res.status(400).json({
        error: payload?.sub_msg || payload?.msg || 'Query failed',
        code: payload?.code || 'UNKNOWN'
      });
    }

    res.json(payload);
  } catch (error) {
    res.status(500).json({ error: error.message || 'Failed to query Alipay trade' });
  }
});

app.post('/gateway/refund', async (req, res) => {
  try {
    const { out_trade_no, refund_amount, out_request_no, refund_reason } = req.body;
    if (!out_trade_no || !refund_amount || !out_request_no) {
      return res.status(400).json({ error: 'Missing refund fields' });
    }

    if (MOCK) {
      return res.json({ out_trade_no, refund_amount, status: 'REFUND_SUCCESS' });
    }

    const result = await alipaySdk.exec('alipay.trade.refund', {
      bizContent: {
        out_trade_no,
        refund_amount,
        out_request_no,
        refund_reason: refund_reason || '用户申请退款'
      }
    });

    const payload = result.alipay_trade_refund_response || result;
    if (!payload || payload.code !== '10000') {
      return res.status(400).json({
        error: payload?.sub_msg || payload?.msg || 'Refund failed',
        code: payload?.code || 'UNKNOWN'
      });
    }
    res.json(payload);
  } catch (error) {
    res.status(500).json({ error: error.message || 'Failed to refund trade' });
  }
});

app.post('/gateway/refund-query', async (req, res) => {
  try {
    const { out_trade_no, out_request_no } = req.body;
    if (!out_trade_no || !out_request_no) {
      return res.status(400).json({ error: 'Missing refund query fields' });
    }

    if (MOCK) {
      return res.json({ out_trade_no, out_request_no, status: 'REFUND_SUCCESS' });
    }

    const result = await alipaySdk.exec('alipay.trade.fastpay.refund.query', {
      bizContent: {
        out_trade_no,
        out_request_no
      }
    });

    const payload = result.alipay_trade_fastpay_refund_query_response || result;
    if (!payload || payload.code !== '10000') {
      return res.status(400).json({
        error: payload?.sub_msg || payload?.msg || 'Refund query failed',
        code: payload?.code || 'UNKNOWN'
      });
    }
    res.json(payload);
  } catch (error) {
    res.status(500).json({ error: error.message || 'Failed to query refund status' });
  }
});

app.post('/gateway/verify-notify', (req, res) => {
  if (MOCK) {
    return res.json({ verified: true });
  }
  try {
    const verified = alipaySdk.checkNotifySign(req.body);
    res.json({ verified });
  } catch (error) {
    res.status(500).json({ error: error.message || 'Failed to verify notify sign' });
  }
});

const port = Number(process.env.ALIPAY_GATEWAY_PORT || 3000);
app.listen(port, () => {
  console.log(`Alipay gateway listening on http://127.0.0.1:${port} [${MOCK ? 'MOCK' : 'LIVE'}]`);
});
