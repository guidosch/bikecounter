# Execute Cloud Function Locally

## Setup

1. **Install dependencies:**
   ```bash
   npm install
   ```

2. **Download the Firebase/Google Cloud service account key:**
   - Go to the [Google Cloud Console](https://console.cloud.google.com/)
   - Navigate to **IAM & Admin → Service Accounts**
   - Select your project and the service account used by your Cloud Function
   - Click **Keys → Add Key → Create new key → JSON**
   - Save the downloaded file as `service-account-key.json` in this directory

3. **Run the function locally:**
   ```bash
   npm start
   ```
   The function will be available at `http://localhost:8080/`

4. **Test with a sample request:**
   ```bash
   curl -X POST http://localhost:8080 \
     -H "Content-Type: application/json" \
     -d @test-payload.json
   ```