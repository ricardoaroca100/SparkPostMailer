<?php


/**
 * Class SparkpostMailer
 */
class SparkpostMailer
{

    /** Sparkpost API key
     * @var string
     */
    public $API_KEY = '';

    /** API URL
     * @var string
     */
    private $API_URL = 'https://api.sparkpost.com/api/v1/transmissions';

    /**
     * Sets the From email address for the message.
     * @var string
     */
    public $From              = 'root@localhost';

    /**
     * Sets the From name of the message.
     * @var string
     */
    public $FromName          = 'Root User';

    /**
     * Sets the Subject of the message.
     * @var string
     */
    public $Subject           = '';

    /**
     * Sets the Body of the message.  This can be either an HTML or text body.
     * If HTML then run IsHTML(true).
     * @var string
     */
    public $Body              = '';

    /**
     * Sets the text-only body of the message.
     * This body can be read by mail
     * clients that do not have HTML email capability such as mutt. Clients
     * that can read HTML will view the normal Body.
     * @var string
     */
    public $AltBody           = '';


    /**
     * Sets timeout in seconds.
     * @var int
     */
    public $Timeout       = 60;


    private   $to             = array();
    private   $cc             = array();
    private   $bcc            = array();
    private   $ReplyTo        = array();
    private   $all_recipients = array();
    private   $attachment     = array();

    public $result;
    public $debug = 0;
    public $debug_text = '';


    /**
     * SparkpostMailer constructor.
     * Backward compatibility with implementations of PHPMailer
     * @param bool $exceptions
     */
    public function __construct($exceptions = false)
    {
    }

    /**
     * Adds a "To" address.
     * @param $address
     * @param string $name
     * @return bool
     */
    public function AddAddress($address, $name = '') {
        return $this->AddAnAddress('to', $address, $name);
    }

    /**
     * Adds a "Cc" address.
     * @param $address
     * @param string $name
     * @return bool
     */
    public function AddCC($address, $name = '') {
        return $this->AddAnAddress('cc', $address, $name);
    }

    /**
     * Adds a "Bcc" address.
     * @param $address
     * @param string $name
     * @return bool
     */
    public function AddBCC($address, $name = '') {
        return $this->AddAnAddress('bcc', $address, $name);
    }

    /**
     * Adds a "Reply-to" address.
     * @param $address
     * @param string $name
     * @return bool
     */
    public function AddReplyTo($address, $name = '') {
        return $this->AddAnAddress('ReplyTo', $address, $name);
    }

    /**
     *  Set the From and FromName properties
     * @param $address
     * @param string $name
     * @param int $auto
     * @return bool
     */
    public function SetFrom($address, $name = '', $auto=1) {
        $address = trim($address);
        $name = trim(preg_replace('/[\r\n]+/', '', $name)); //Strip breaks and trim
        if (!self::ValidateAddress($address)) {

            return false;
        }
        $this->From = $address;
        $this->FromName = $name;
        if ($auto) {
            if (empty($this->ReplyTo)) {
                $this->AddAnAddress('ReplyTo', $address, $name);
            }
        }
        return true;
    }

    /**
     * Adds an attachment from a path on the filesystem.
     * Returns false if the file could not be found
     * or accessed.
     * @param $path
     * @param string $name
     * @param string $encoding
     * @param string $type
     * @return bool
     */
    public function AddAttachment($path, $name = '', $encoding = 'base64', $type = 'application/octet-stream') {

        $filename = basename($path);
        if ( $name == '' ) {
            $name = $filename;
        }

        $this->attachment[] = array(
            0 => $path,
            1 => $filename,
            2 => $name,
            3 => $encoding,
            4 => $type,
            5 => false,  // isStringAttachment
            6 => 'attachment',
            7 => 0
        );

        return true;
    }


    /**
     * Adds an address to one of the recipient arrays
     * Addresses that have been added already return false, but do not throw exceptions
     * @param string $kind One of 'to', 'cc', 'bcc', 'ReplyTo'
     * @param string $address The email address to send to
     * @param string $name
     * @return boolean true on success, false if address already used or invalid in some way
     * @access private
     */
    private function AddAnAddress($kind, $address, $name = '') {
        if (!preg_match('/^(to|cc|bcc|ReplyTo)$/', $kind)) {
            //echo 'Invalid recipient array: ' . kind;
            return false;
        }
        $address = trim($address);
        $name = trim(preg_replace('/[\r\n]+/', '', $name)); //Strip breaks and trim
        if (!self::ValidateAddress($address)) {
            return false;
        }
        if ($kind != 'ReplyTo') {
            if (!isset($this->all_recipients[strtolower($address)])) {
                array_push($this->$kind, array($address, $name));
                $this->all_recipients[strtolower($address)] = true;
                return true;
            }
        } else {
            if (!array_key_exists(strtolower($address), $this->ReplyTo)) {
                $this->ReplyTo[strtolower($address)] = array($address, $name);
                return true;
            }
        }
        return false;
    }

    /**
     * Check that a string looks roughly like an email address should
     * Static so it can be used without instantiation
     * Tries to use PHP built-in validator in the filter extension (from PHP 5.2), falls back to a reasonably competent regex validator
     * Conforms approximately to RFC2822
     * @link http://www.hexillion.com/samples/#Regex Original pattern found here
     * @param string $address The email address to check
     * @return boolean
     * @static
     * @access public
     */
    public static function ValidateAddress($address) {
        if (function_exists('filter_var')) { //Introduced in PHP 5.2
            if(filter_var($address, FILTER_VALIDATE_EMAIL) === FALSE) {
                return false;
            } else {
                return true;
            }
        } else {
            return preg_match('/^(?:[\w\!\#\$\%\&\'\*\+\-\/\=\?\^\`\{\|\}\~]+\.)*[\w\!\#\$\%\&\'\*\+\-\/\=\?\^\`\{\|\}\~]+@(?:(?:(?:[a-zA-Z0-9_](?:[a-zA-Z0-9_\-](?!\.)){0,61}[a-zA-Z0-9_-]?\.)+[a-zA-Z0-9_](?:[a-zA-Z0-9_\-](?!$)){0,61}[a-zA-Z0-9_]?)|(?:\[(?:(?:[01]?\d{1,2}|2[0-4]\d|25[0-5])\.){3}(?:[01]?\d{1,2}|2[0-4]\d|25[0-5])\]))$/', $address);
        }
    }

    /**
     * Evaluates the message and sets Body and AltBody
     * @param $message
     * @param string $basedir
     */
    public function MsgHTML($message, $basedir = '') {

        $this->Body = $message;
        $textMsg = trim(strip_tags(preg_replace('/<(head|title|style|script)[^>]*>.*?<\/\\1>/s','',$message)));
        if (!empty($textMsg) && empty($this->AltBody)) {
            $this->AltBody = html_entity_decode($textMsg);
        }
        if (empty($this->AltBody)) {
            $this->AltBody = 'To view this email message, open it in a program that understands HTML!' . "\n\n";
        }
    }


    /** Sends email
     * @return bool
     */
    public function Send() {


        // CREAR EL PAYLOAD PROCESANDO LOS ELEMENTOS CONFIGURADOS PARA EL ENVÍO

        // * From
        $from = $this->From;
        if($this->FromName) {
            $from = array(
                'email' => $this->From,
                'name' => $this->FromName
            );
        }


        // * Recipients

        $recipients = array();
        $headers_cc = array();
        $header_to = '';

        // Sets first "to", as main recipient (header_to)
        $item = current($this->to);
        $header_to = $item[0];

        // ** Process "to"
        foreach($this->to as $item){
            $address = array(
                'email' => $item[0],
                'header_to' => $header_to
            );
            $address = $item[1] ? $address + array('name' => $item[1]) : $address;

            $recipients[] = array(
                'address' => $address
            );
        }

        // ** Process "cc"
        if(count($this->cc)) {
            foreach ($this->cc as $item) {
                $address = array(
                    'email' => $item[0],
                    'header_to' => $header_to
                );
                $address = $item[1] ? $address + array('name' => $item[1]) : $address;

                $recipients[] = array(
                    'address' => $address
                );
                $headers_cc[] = $item[0];
            }
        }

        // ** Processs "bcc"
        if(count($this->bcc)) {
            foreach ($this->bcc as $item) {
                $address = array(
                    'email' => $item[0],
                    'header_to' => $header_to
                );
                $address = $item[1] ? $address + array('name' => $item[1]) : $address;

                $recipients[] = array(
                    'address' => $address
                );
            }
        }

        // ** Process "reply_to"
        $reply_to = '';
        if (count($this->ReplyTo)) {
            $item = current($this->ReplyTo);
            if ($item[1]) {
                $reply_to = $item[1] . ' <' . $item[0] .'>';
            } else {
                $reply_to = $item[0];
            }
        }

        // * Configure Headers
        $headers = new stdClass();
        if(count($headers_cc)) {
            $headers->CC = implode(',', $headers_cc);
        }


        // * Set Attachments
        $attachments = array();
        if(count($this->attachment)) {
            foreach($this->attachment as $item) {
                $attachments[] = array(
                    'name' => $item[1],
                    'type' => $item[4],
                    'data' => base64_encode(file_get_contents($item[0]))
                );
            }
        }

        // * Create payload
        $payload = array(
            'options' => array(
                'click_tracking' => false,
                'inline_css' => true,
                'transactional' => true,
            ),
            'recipients' => $recipients,
            'content' => array(
                'from' => $from,
                'subject' => $this->Subject,
                'text' => $this->AltBody,
                'html' => $this->Body,
                'headers' => $headers,
                'attachments' => $attachments
            )
        );
        if($reply_to){
            $payload['content']['reply_to'] = $reply_to;
        }

        $payload = $this->payload_to_utf8($payload);

        if($this->debug) {
            $debug_payload = $payload;
            if($debug_payload['content']['attachments']) {
                foreach($debug_payload['content']['attachments'] as &$item){
                    $item['data'] = 'hidden';
                }
            }
            $this->debug_text .= "\n" . str_repeat ('-', 80) . "\nPAYLOAD:\n" . str_repeat ('-', 80) . "\n\n" . print_r($debug_payload, 1) . "\n\n";
        }


        // DO REQUEST

        $headers = array(
            "Content-Type: application/json",
            'Authorization: '. $this->API_KEY
        );

        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $this->API_URL);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, $this->Timeout);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload));
        curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
        curl_setopt ($ch, CURLOPT_SSLVERSION, 6); // Force TLS 1.2

        if($this->debug) {
            $curl_log = tmpfile();
            curl_setopt($ch, CURLOPT_VERBOSE, 1);
            curl_setopt($ch, CURLOPT_STDERR, $curl_log);
        }


        // PROCESS RESPONSE

        $response = curl_exec($ch);
        $this->result = json_decode($response);


        if($this->debug) {
            rewind($curl_log);
            $output = fread($curl_log, 2048);
            $this->debug_text .= "\n" . str_repeat ('-', 80) . "\nREQUEST VERBOSE:\n" . str_repeat ('-', 80) . "\n\n" . print_r($output, 1) . "\n\n"  ;
            fclose($curl_log);

            $this->debug_text .= "\n" . str_repeat ('-', 80) . "\nRESULT:\n" .  str_repeat ('-', 80) . "\n\n" . print_r($this->result, 1) . "\n\n";
        }

        $res = false;
        if (!isset($this->result)):
            $res = false;
        elseif (isset($this->result->errors)):
            $res = false;
        else:
            $res = true;
        endif;

        curl_close($ch);

        return $res;

    }

    /** Convert payload strings to utf8
     * @param $payload
     * @return array
     */
    private function payload_to_utf8($payload) {

        foreach($payload['recipients'] as &$item) {

            $item['email'] = mb_detect_encoding($item['email'], 'UTF-8', true) === false ? utf8_encode($item['email']) : $item['email'];

            if(array_key_exists('header_to', $item)) {
                $item['header_to'] = mb_detect_encoding($item['header_to'], 'UTF-8', true) === false ? utf8_encode($item['header_to']) : $item['header_to'];
            }
            if(array_key_exists('name', $item)) {
                $item['name'] = mb_detect_encoding($item['name'], 'UTF-8', true) === false ? utf8_encode($item['name']) : $item['name'];
            }
        }

        $payload['content']['from']['email'] = mb_detect_encoding($payload['content']['from']['email'], 'UTF-8', true) === false ? utf8_encode($payload['content']['from']['email']) : $payload['content']['from']['email'];
        if(array_key_exists('name', $payload['content']['from'])) {
            $payload['content']['from']['name'] = mb_detect_encoding($payload['content']['from']['name'], 'UTF-8', true) === false ? utf8_encode($payload['content']['from']['name']) : $payload['content']['from']['name'];
        }

        $payload['content']['subject'] = mb_detect_encoding($payload['content']['subject'], 'UTF-8', true) === false ? utf8_encode($payload['content']['subject']) : $payload['content']['subject'];

        $payload['content']['text'] = mb_detect_encoding($payload['content']['text'], 'UTF-8', true) === false ? utf8_encode($payload['content']['text']) : $payload['content']['text'];
        $payload['content']['html'] = mb_detect_encoding($payload['content']['html'], 'UTF-8', true) === false ? utf8_encode($payload['content']['html']) : $payload['content']['html'];


        if($payload->headers->CC) {
            $item['name'] = mb_detect_encoding($payload->headers->CC, 'UTF-8', true) === false ? utf8_encode($payload->headers->CC) : $payload->headers->CC;
        }

        if($payload['content']['reply_to']) {
            $payload['content']['reply_to'] = mb_detect_encoding($payload['content']['reply_to'], 'UTF-8', true) === false ? utf8_encode($payload['content']['reply_to']) : $payload['content']['reply_to'];
        }

        return $payload;

    }


}
