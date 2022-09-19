const nodemailer = require('nodemailer');
const mg = require('nodemailer-mailgun-transport');

// This is your API key that you retrieve from www.mailgun.com/cp (free up to 10K monthly emails)
const auth = {
  auth: {
    api_key: 'xxx',
    domain: 'xxx'
  }
}

const nodemailerMailgun = nodemailer.createTransport(mg(auth));

function sendMail(){
  nodemailerMailgun.sendMail({
    from: 'xxx',
    to: 'xxx', // An array if you have multiple recipients.
    subject: 'Bikecounter device alert',
    html: '<b>test</b>',
    text: 'Das folgende device ist offline, bzw. low batt'
  }, (err, info) => {
    if (err) {
      console.log(`Error: ${err}`);
    }
    else {
      console.log(`Response: ${info}`);
    }
  });
}



/**
 * Responds to any HTTP request.
 *
 * @param {!express:Request} req HTTP request context.
 * @param {!express:Response} res HTTP response context.
 */
exports.checkHealthData = (req, res) => {
  let message = req.query.message || req.body.message || 'Hello World!';
  sendMail();
  res.status(200).send(message);
};
